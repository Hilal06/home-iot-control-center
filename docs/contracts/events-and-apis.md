# Kontrak Data & Spesifikasi Antarmuka (Events & APIs)

Dokumen ini mendefinisikan skema kontrak data JSON antar-layanan (*inter-service communication contracts*), baik melalui **Redis Pub/Sub**, antrean **MQTT**, maupun antarmuka **HTTP REST**.

---

## 1. Kontrak Event Redis Pub/Sub

Lapisan Rust Edge Ingestion Gateway mempublikasikan event ke Redis dengan format JSON standar yang dikonsumsi oleh Spring Boot Business Engine.

### 1.1. Channel: `iot:telemetry:stream`
Dipublikasikan setiap kali paket telemetri baru diterima dari ESP32 (setiap ~1000ms).

```json
{
  "device_id": "esp32_master",
  "timestamp": 1730800000000,
  "pv_array_voltage": 68.5,
  "pv_array_current": 12.4,
  "pv_power_watts": 850.0,
  "solar_yield_kwh": 7.45,
  "panel_temp_c": 38.5,
  "solar_charger_active": true,
  "mains_voltage": 223.5,
  "house_power_w": 340.0,
  "ups_battery_pct": 98.0,
  "ups_load_pct": 14.2,
  "rack_temp_c": 24.2,
  "temp_outdoor_c": 29.0,
  "humidity_pct": 58.0,
  "water_leak": false,
  "ping_gateway_ms": 2.8,
  "rssi_dbm": -52,
  "esp_free_heap_kb": 226,
  "garage_status": false,
  "kitchen_breaker": true,
  "hvac_power": true,
  "hvac_target_temp": 22,
  "gas_ppm": 145,
  "water_level_pct": 82,
  "ultrasonic_distance_cm": 18.4,
  "lamps": {
    "living_room": true,
    "main_room": false,
    "garage": false,
    "kitchen": true,
    "stair_garage": false,
    "stair_fl2": false,
    "yard_front": true,
    "yard_side": false,
    "yard_back": false,
    "server_room": true,
    "electrical_room": false
  }
}
```

### 1.2. Channel: `iot:device:status`
Dipublikasikan saat status perangkat berubah (misalnya melalui MQTT LWT atau watchdog timeout).

```json
{
  "device_id": "esp32_master",
  "status": "OFFLINE",
  "reason": "MQTT_LWT_DISCONNECT",
  "timestamp": 1730800005000
}
```

### 1.3. Channel: `iot:scene:events`
Dipublikasikan oleh Spring Boot saat suatu scene otomasi diaktifkan, memungkinkan layanan lain (seperti Rust gateway atau WebSocket hub) mendistribusikan notifikasi event.

```json
{
  "scene_name": "NIGHT_MODE",
  "activated_at": "2026-11-11T05:00:00Z",
  "description": "Lampu luar menyala, beban interior diminimalkan"
}
```

---

## 2. Kontrak Perintah Aktuator (MQTT Commands)

Format payload untuk pengiriman instruksi ke perangkat keras fisik melalui MQTT broker:
- **Topik Broadcast Master**: `home/actuators/control`
- **Topik Perangkat Spesifik**: `home/actuators/{device_id}/control`

### 2.1. Kontrol Lampu (`set_lamp`)
```json
{
  "command": "set_lamp",
  "room": "yard_front",
  "state": true,
  "source": "SCHEDULE",
  "force": false
}
```

### 2.2. Kontrol Beban Rumah & Aktuator (`set_hvac`, `set_breaker`, `set_garage`)
```json
{
  "command": "set_hvac",
  "power": true,
  "target_temp": 24,
  "source": "MANUAL",
  "force": false
}
```

### 2.3. Perintah Darurat & Pemutus Arus (`trigger_emergency`)
```json
{
  "command": "trigger_emergency",
  "alarm": true,
  "cutoff": true,
  "reason": "GAS_LEAK_DETECTED_EXCEED_300PPM",
  "source": "AUTOMATION",
  "force": true
}
```

---

## 3. Spesifikasi REST API (Spring Boot Business Engine - Port 8081)

### 3.1. Ringkasan Energi & Akuntansi Tarif
`GET /api/v1/business/analytics/energy`

**Respon Sukses (`200 OK`)**:
```json
{
  "device_id": "esp32_master",
  "pln_contract_limit_watts": 900.0,
  "tariff_rate_idr_per_kwh": 1352.0,
  "daily_pln_kwh": 4.12,
  "daily_pln_cost_idr": 5570.24,
  "daily_solar_yield_kwh": 8.75,
  "daily_solar_savings_idr": 11830.00,
  "solar_self_sufficiency_pct": 68.0,
  "current_load_w": 340.0,
  "current_solar_w": 850.0,
  "status": "NORMAL"
}
```

### 3.2. Status Aturan Keselamatan & Proteksi
`GET /api/v1/business/rules/status`

**Respon Sukses (`200 OK`)**:
```json
{
  "system_state": "HEALTHY",
  "pln_overload_protection": {
    "active": true,
    "limit_w": 900.0,
    "current_w": 340.0,
    "headroom_w": 560.0
  },
  "hazard_monitors": {
    "gas_mq_ppm": 145,
    "gas_status": "NORMAL",
    "water_leak_detected": false
  },
  "solar_optimization": {
    "battery_pct": 98.0,
    "surplus_detected": true
  }
}
```

### 3.3. Device Registry Management
Mengelola daftar perangkat IoT yang terdaftar, status konektivitas, serta pemetaan ruangan.

- **Daftar Semua Perangkat**: `GET /api/v1/business/devices`
- **Detail Perangkat**: `GET /api/v1/business/devices/{id}`
- **Pembaruan Metadata (Nama/Ruangan)**: `PUT /api/v1/business/devices/{id}`
- **Hapus Perangkat**: `DELETE /api/v1/business/devices/{id}`

**Payload Pembaruan Metadata (`PUT`)**:
```json
{
  "name": "ESP32 Gateway Lantai 1",
  "room": "Lantai 1 Utama"
}
```

**Respon Sukses (`200 OK`)**:
```json
[
  {
    "id": "esp32_master",
    "name": "ESP32 Gateway Lantai 1",
    "room": "Lantai 1 Utama",
    "type": "RELAY",
    "status": "ONLINE",
    "lastSeen": "2026-11-11T05:30:00Z"
  }
]
```

### 3.4. Dynamic Scene Engine
Menjalankan dan mengontrol preset otomasi beban rumah tangga secara simultan.

- **Status Scene Aktif**: `GET /api/v1/business/scenes`
- **Aktivasi Scene**: `POST /api/v1/business/scenes/{sceneName}/activate`

**Preset Scene yang Didukung**:
- `ALL_OFF`: Memadamkan seluruh lampu dan beban non-kritis.
- `DAY_MODE`: Mode efisiensi siang hari dengan pemanfaatan solar yield.
- `NIGHT_MODE`: Mode malam dengan penerangan eksterior dan penghematan interior.
- `SOLAR_PRIORITY`: Mengutamakan konsumsi daya dari solar inverter dan baterai UPS.

**Respon Sukses (`200 OK`)**:
```json
{
  "current_scene": "NIGHT_MODE",
  "available_scenes": [
    "ALL_OFF",
    "DAY_MODE",
    "NIGHT_MODE",
    "SOLAR_PRIORITY"
  ],
  "activated_at": "2026-11-11T05:30:15Z",
  "description": "Lampu luar menyala, beban interior diminimalkan"
}
```

### 3.5. PLN Prepaid Token & Burn-Rate Tracker
Pelacakan sisa pulsa listrik PLN prabayar (token), burn rate harian, estimasi sisa hari pemakaian, dan isi ulang token.

- **Status & Estimasi Token**: `GET /api/v1/business/token/status`
- **Isi Ulang Token**: `POST /api/v1/business/token/recharge`

**Payload Isi Ulang Token (`POST`)**:
Dapat mengisi berdasarkan nominal Rupiah (`amount_idr`) atau langsung kWh (`added_kwh`):
```json
{
  "amount_idr": 100000.0,
  "added_kwh": null
}
```

**Respon Sukses (`200 OK`)**:
```json
{
  "remaining_kwh": 65.42,
  "remaining_idr": 88447.84,
  "tariff_per_kwh": 1352.0,
  "daily_burn_rate_kwh": 4.12,
  "estimated_days_remaining": 15.87,
  "is_low_balance": false,
  "low_balance_threshold_kwh": 20.0
}
```

---

## 4. Taksonomi Kode Kesalahan (Error Taxonomy)

| Kode HTTP | Makna | Penyebab |
| :--- | :--- | :--- |
| `200 OK` | Sukses | Permintaan berhasil diproses. |
| `400 Bad Request` | Payload Tidak Valid | Format JSON salah atau parameter wajib hilang. |
| `423 Locked` | Device Locked | Perangkat dalam keadaan terkunci (`is_locked: true`) dan perintah dikirim tanpa flag `force: true`. |
| `503 Service Unavailable` | Gateway / Broker Down | Broker MQTT tidak terjangkau dan fallback gagal. |

---

## 5. Referensi Dokumentasi Resmi

1. **JSON Schema Specification**: [https://json-schema.org/](https://json-schema.org/)
2. **RFC 6585 - Additional HTTP Status Codes (HTTP 423 Locked)**: [https://datatracker.ietf.org/doc/html/rfc6585](https://datatracker.ietf.org/doc/html/rfc6585)
3. **MQTT v5.0 / v3.1.1 OASIS Specification**: [https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
4. **Redis Pub/Sub Documentation**: [https://redis.io/docs/latest/develop/interact/pubsub/](https://redis.io/docs/latest/develop/interact/pubsub/)

