# Layanan: Spring Boot Business Engine (`Service_Business_Java`)

Dokumen teknis ini menjelaskan arsitektur, struktur paket, logika aturan keselamatan (*Safety Rule Engine*), akuntansi energi, pengelolaan profil rumah (*Scene Engine*), pelacak token listrik PLN, inventaris perangkat, dan notifikasi instan pada **Spring Boot Business Engine** (Port `8081`).

---

## 1. Peran & Tanggung Jawab Layanan

Layanan Spring Boot bertanggung jawab atas logika bisnis, akuntansi energi, dan integrasi pihak ketiga:

1. **Redis Telemetry & Status Consumer**:
   - Mendengarkan channel `iot:telemetry:stream` untuk memproses setiap paket telemetri sensor.
   - Mendengarkan channel `iot:device:status` untuk memantau status *Last Will and Testament* (LWT) perangkat.
2. **Safety Rule Engine (Interlock & Proteksi Beban)**:
   - *PLN 900W Overload Guard*: Memantau beban rumah. Memicu peringatan jika mendekati batas 900W (> 860W).
   - *Gas & Water Leak Safety Cutoff*: Mengirim sinyal bahaya kritis jika gas MQ > 300 ppm atau terdeteksi kebocoran air aktif.
   - *Solar Surplus Manager*: Mendeteksi surplus pembangkitan surya (> 300W di atas beban dengan baterai > 95%).
3. **Notification Dispatcher (Telegram & Discord)**:
   - Mengirim notifikasi bahaya secara *real-time* ke Telegram Bot dan Discord Webhook menggunakan `RestClient` bawaan Spring.
   - Memiliki proteksi *anti-spam throttling* (jendela pendinginan 60 detik per jenis alert).
   - Berjalan dalam mode aman (*Safe No-Op*) jika kredensial bot belum dikonfigurasi.
4. **Device Registry Service**:
   - Mengelola metadata dan kehadiran perangkat (ID, nama ramah pengguna, ruangan/zona, tipe, status `ONLINE`/`OFFLINE`).
   - Disimpan persisten di **Redis Hash** (`iot:registry:devices`) dengan sinkronisasi *cache* lokal instan.
5. **Dynamic Scene Engine**:
   - Mengelola profil mode rumah tangga: **`NORMAL`**, **`AWAY`** (mematikan lampu dalam & proteksi keamanan diperketat), dan **`SLEEP`** (meredupkan lampu utama).
   - Menyimpan status di Redis (`iot:scene:current`) dan menerbitkan event transisi ke `iot:scene:events`.
6. **PLN Token Tracker & Forecaster**:
   - Melacak sisa pulsa meteran listrik prabayar (kWh dan Rupiah).
   - Memotong saldo kWh secara presisi saat telemetri masuk berdasarkan daya riil jaringan PLN.
   - Menghitung *moving-average burn rate* dan memprediksi sisa hari pemakaian sebelum pulsa padam.
   - Memicu peringatan bot otomatis jika saldo tersisa < 15 kWh.
   - Mendukung isi ulang (*recharge*) via nominal Rupiah atau penambahan kWh langsung.
7. **Akuntansi Energi & Penjadwalan Lampu**:
   - Akumulasi pemakaian listrik harian dan penghematan biaya dari panel surya.
   - Penjadwalan cron penyalaan lampu luar (18:00) dan pemadaman (05:30).

---

## 2. Struktur Paket Java (Modern Java 17 Records & Clean Architecture)

```
Service_Business_Java/src/main/java/com/homeiot/business/
├── BusinessEngineApplication.java       # Spring Boot Application Entrypoint
├── config/
│   ├── RedisConfig.java                 # Konfigurasi koneksi Redis & MessageListenerContainer
│   └── WebConfig.java                   # Konfigurasi CORS & REST Web Layer
├── consumer/
│   └── TelemetryRedisConsumer.java      # Listener Redis Pub/Sub (Telemetry stream & Device status)
├── controller/
│   ├── AnalyticsController.java         # REST API analitik energi & ringkasan aturan
│   ├── DeviceController.java            # REST API inventaris & metadata perangkat
│   ├── HealthController.java            # Health check endpoint
│   ├── PlnTokenController.java          # REST API monitoring & isi ulang token PLN
│   └── SceneController.java             # REST API kontrol & aktivasi mode rumah (Scenes)
├── device/
│   ├── Device.java                      # Record representasi perangkat IoT
│   └── DeviceRegistryService.java       # Service pengelola inventaris tersimpan di Redis Hash
├── domain/
│   ├── DeviceStatusEvent.java           # Record event perubahan status koneksi (LWT)
│   ├── EnergySummary.java               # Record ringkasan akuntansi energi harian
│   └── TelemetryPayload.java            # Record ringkas telemetri sensor (Ignore unknown attributes)
├── notification/
│   ├── AlertEvent.java                  # Record payload notifikasi & tingkat keparahan
│   └── NotificationService.java         # HTTP client dispatching ke Telegram & Discord
├── rules/
│   ├── RuleEvaluationResult.java        # Record hasil evaluasi proteksi keselamatan
│   └── SafetyRuleEngine.java            # Evaluator aturan proteksi beban, gas, air, & surya
├── scene/
│   ├── SceneEngineService.java          # Service pengelola transisi mode rumah tangga
│   └── SceneStatus.java                 # Record status scene aktif
├── scheduler/
│   └── LampAutomationScheduler.java     # Timer jadwal lampu harian (@Scheduled)
├── service/
│   └── EnergyAnalyticsService.java      # Kalkulator konsumsi kWh, solar yield, dan tarif PLN
└── token/
    ├── PlnTokenService.java             # Service pelacak saldo token & prediksi burn-rate
    ├── RechargeRequest.java             # Record permintaan top-up token
    └── TokenStatus.java                 # Record status saldo & proyeksi ketahanan pulsa
```

---

## 3. Detail Implementasi Logika Kunci

<details>
<summary><strong>1. Safety Rule Engine & Alert Dispatcher</strong> (Klik untuk melihat)</summary>

```java
// Contoh integrasi evaluasi aturan dan pemicuan notifikasi
RuleEvaluationResult eval = safetyRuleEngine.evaluate(payload);

if (eval.isGasHazardTriggered()) {
    notificationService.dispatch(AlertEvent.of(
            "GAS_HAZARD",
            "Bahaya Kebocoran Gas!",
            String.format("Konsentrasi gas %d ppm melebihi ambang batas aman %d ppm!",
                    payload.gasPpmVal(), safetyRuleEngine.getGasPpmThreshold()),
            AlertEvent.AlertLevel.CRITICAL
    ));
}
```
</details>

<details>
<summary><strong>2. Dynamic Scene Engine</strong> (Klik untuk melihat)</summary>

```java
// Aktivasi Scene dan Broadcast Event
public SceneStatus activateScene(String sceneName) {
    String normalized = sceneName.trim().toUpperCase(Locale.ROOT);
    this.currentScene = normalized;
    this.activatedAt = Instant.now();

    // Simpan di Redis & publish event
    redisTemplate.opsForValue().set("iot:scene:current", normalized);
    redisTemplate.convertAndSend("iot:scene:events", normalized);

    // Kirim notifikasi bot
    notificationService.dispatch(AlertEvent.of(
            "SCENE_CHANGE_" + normalized,
            "Mode Rumah Diubah",
            "🏠 [MODE " + normalized + "] Profil operasi rumah berhasil diubah.",
            AlertEvent.AlertLevel.INFO
    ));

    return getCurrentSceneStatus();
}
```
</details>

<details>
<summary><strong>3. PLN Token Tracker & Burn-Rate Forecaster</strong> (Klik untuk melihat)</summary>

```java
// Pengurangan kuota dan proyeksi hari tersisa
public synchronized void deduct(float netPlnWatts, float dtHours) {
    float kwhBurned = (netPlnWatts * dtHours) / 1000.0f;
    this.remainingKwh = Math.max(0.0f, this.remainingKwh - kwhBurned);

    // Moving average burn-rate
    float currentDailyProjection = (netPlnWatts * 24.0f) / 1000.0f;
    this.dailyBurnRateKwh = (this.dailyBurnRateKwh * 0.95f) + (currentDailyProjection * 0.05f);

    // Alert saat saldo kritis < 15 kWh
    if (this.remainingKwh < lowBalanceThresholdKwh) {
        notificationService.dispatch(AlertEvent.of(
                "PLN_LOW_TOKEN",
                "Peringatan Pulsa Listrik PLN Kritis!",
                String.format("Sisa pulsa listrik PLN %.1f kWh. Segera isi ulang!", this.remainingKwh),
                AlertEvent.AlertLevel.WARNING
        ));
    }
}
```
</details>

---

## 4. Konfigurasi Lingkungan (`application.yml`)

```yaml
server:
  port: 8081

spring:
  application:
    name: home-iot-business-engine
  data:
    redis:
      host: ${REDIS_HOST:localhost}
      port: ${REDIS_PORT:6379}

iot:
  rules:
    pln-limit-watts: 900.0
    pln-warning-threshold: 860.0
    gas-ppm-threshold: 300
    pln-tariff-per-kwh: 1352.0
  notification:
    cooldown-seconds: 60
    telegram:
      bot-token: ${TELEGRAM_BOT_TOKEN:}
      chat-id: ${TELEGRAM_CHAT_ID:}
    discord:
      webhook-url: ${DISCORD_WEBHOOK_URL:}
  token:
    low-balance-threshold-kwh: 15.0
```
