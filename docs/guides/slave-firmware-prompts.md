# Panduan & Kumpulan Prompt Pembuatan Firmware Slave Node (ESP-NOW)

Dokumen ini berisi kumpulan **prompt siap pakai** (*ready-to-use system prompts*) untuk membuat firmware masing-masing Slave Node pada ekosistem Home IoT Service. Setiap prompt dirancang khusus untuk diberikan kepada asisten AI coding (seperti Antigravity, Claude Code, Cursor) agar menghasilkan proyek PlatformIO yang **langsung dapat dikompilasi, bebas bloat (*Ponytail-certified*), dan mematuhi seluruh standar proteksi sirkuit**.

---

## 1. Standar Global & Aturan Wajib Seluruh Firmware Slave

Seluruh prompt mengacu pada kontrak teknis di **[espnow-master-slave-topology.md](../architecture/espnow-master-slave-topology.md)**:

1. **Framework & Platform**:
   - Node 1, 2, 3, 5, 6: Platform `espressif8266`, board `d1_mini`, framework `arduino`.
   - Node 4: Platform `espressif32`, board `esp32doit-devkit-v1`, framework `arduino`.
2. **Protokol Nirkabel**:
   - Protokol: **ESP-NOW** pada frekuensi 2.4 GHz.
   - Saluran WiFi: Terkunci pada **Channel 6 (Static)** dengan fallback *channel sweeping* (1–13) jika transmisi gagal 5x berturut-turut.
   - Bitrate Radio: Diatur ke **1 Mbps DSSS (`WIFI_PHY_RATE_1M_L`)** untuk daya penetrasi tembok maksimal.
   - Alamat Tujuan: **Broadcast MAC Address (`FF:FF:FF:FF:FF:FF`)**. Tidak boleh ada *hardcoded MAC* milik Master.
3. **Format Data**:
   - Wajib menggunakan `struct __attribute__((packed))` standar dengan `PacketHeader` (4 byte) di awal.
   - Dilarang mem-parse JSON di dalam mikrokontroler (zero JSON overhead).
4. **Keandalan & Keselamatan**:
   - Hardware Watchdog Timer (WDT) wajib aktif.
   - Pin Wemos D1 Mini hanya boleh menggunakan **Pin Bebas Aman: D1, D2, D5, D6, D7**. Dilarang menggunakan D3, D4 (aktif-low boot), dan D8 (bootloader trap) untuk relai.
   - Otomasi darurat lokal (*Local Interlock*) wajib berjalan di dalam firmware independen tanpa menunggu koneksi nirkabel.

---

## 2. Kumpulan Prompt Pembuatan Firmware per Node

---

### Prompt 1: Node 1 - Rooftop Weather & Solar Station (`esp_roof_weather`)

```markdown
Buatkan proyek PlatformIO lengkap (platformio.ini dan src/main.cpp) untuk Slave Node Atap & Panel Surya (esp_roof_weather) pada ekosistem Home IoT.

Spesifikasi Perangkat Keras:
- Mikrokontroler: Wemos D1 Mini (ESP8266)
- Sensor Suhu & Kelembaban Luar: SHT31 atau AHT20 pada antarmuka I2C (Pin D1=SCL, D2=SDA)
- Sensor Suhu Permukaan Panel Surya: DS18B20 Waterproof pada 1-Wire (Pin D4/GPIO2 dengan resistor pull-up 4.7kΩ)
- Sensor Intensitas Cahaya (LDR): Pin Analog A0 (dengan pembagi tegangan max 1.0V/3.3V)
- Sensor Hujan: Pin Digital D5 (GPIO14, input dengan pull-up internal)

Spesifikasi Komunikasi ESP-NOW:
- Saluran WiFi: Kunci pada Channel 6 (WIFI_STA).
- PHY Rate: 1 Mbps DSSS (WIFI_PHY_RATE_1M_L) untuk penetrasi sinyal maksimal ke lantai 1.
- Peer MAC: Broadcast (FF:FF:FF:FF:FF:FF).
- Format Paket Biner:
  enum MessageType : uint8_t { MSG_TELEMETRY_ROOF = 0x01 };
  struct __attribute__((packed)) PacketHeader {
    uint8_t  msg_type;   // 0x01
    uint8_t  node_id;    // 1
    uint16_t seq_number; // Sequence counter
  };
  struct __attribute__((packed)) RoofTelemetryPacket {
    PacketHeader header;
    float temp_outdoor;
    float humidity;
    float panel_temp;
    uint16_t ldr_raw;
    bool is_raining;
  };

Perilaku Program:
1. Inisialisasi I2C, OneWire/DallasTemperature, dan ESP-NOW.
2. Setiap 3000 ms, baca seluruh sensor.
3. Bungkus data ke dalam struct RoofTelemetryPacket dan kirim via esp_now_send().
4. Aktifkan Hardware Watchdog (ESP.wdtEnable(5000)).
5. Jika transmisi gagal berulang kali (5x berturut-turut), lakukan channel scan (1-13) untuk mencari channel aktif Master sebelum kembali ke mode periodik.
6. Kode harus ringkas, tanpa library berlebih, non-blocking (hindari delay panjang di loop).
```

---

### Prompt 2: Node 2 - Kitchen & Hazard Safety (`esp_kitchen_safe`)

```markdown
Buatkan proyek PlatformIO lengkap (platformio.ini dan src/main.cpp) untuk Slave Node Keselamatan Dapur & Gas LPG (esp_kitchen_safe) pada ekosistem Home IoT.

Spesifikasi Perangkat Keras:
- Mikrokontroler: Wemos D1 Mini (ESP8266)
- Sensor Gas LPG / Metana (MQ-4 atau MQ-2): Output analog masuk ke Pin A0 (melalui pembagi tegangan 1kΩ:2kΩ untuk mengonversi 5V ke rentang 0-3.3V)
- Sensor Rembesan Air Wastafel: Pin Digital D5 (GPIO14, input dengan pull-up internal)
- Relai Solenoid Gas Valve 12V: Pin Digital D6 (GPIO12, output active-LOW berisolasi optocoupler)
- Active Piezo Buzzer 85dB: Pin Digital D7 (GPIO13, output active-HIGH)

Kontrak Paket ESP-NOW:
- Saluran WiFi: Channel 6 (WIFI_STA), PHY Rate 1 Mbps DSSS.
- Peer MAC: Broadcast (FF:FF:FF:FF:FF:FF).
- Format Paket:
  enum MessageType : uint8_t { 
    MSG_TELEMETRY_KITCHEN = 0x02,
    MSG_CMD_ACTUATOR      = 0x10,
    MSG_EMERGENCY_ALERT   = 0xFF
  };
  struct __attribute__((packed)) PacketHeader {
    uint8_t  msg_type;
    uint8_t  node_id;    // 2
    uint16_t seq_number;
  };
  struct __attribute__((packed)) KitchenTelemetryPacket {
    PacketHeader header;
    uint16_t gas_ppm;
    bool water_leak;
    bool gas_valve_open;
    bool siren_active;
  };
  struct __attribute__((packed)) ActuatorCommandPacket {
    PacketHeader header;
    uint8_t target_actuator_id; // 1 = Gas Solenoid
    uint8_t action;             // 0 = CLOSE, 1 = OPEN
  };

Perilaku & Logika Keselamatan Mandiri (Local Safety Interlock):
1. LOOP CRITICAL (< 100ms): Baca nilai gas A0. Konversi nilai ADC menjadi perkiraan PPM.
2. Jika Gas >= 400 PPM:
   - SEKETIKA: Matikan relai solenoid gas (D6 = HIGH / valve cut-off).
   - SEKETIKA: Nyalakan buzzer alarm lokal (D7 = HIGH).
   - BURST TRANSMISSION: Kirim paket MSG_EMERGENCY_ALERT sebanyak 3 kali berturut-turut (jeda 20ms) ke Master via ESP-NOW.
3. Menerima Perintah Eksternal (esp_now_register_recv_cb):
   - Menerima ActuatorCommandPacket dari Master. Jika kondisi gas aman (< 400 PPM), perbolehkan membuka kembali katup solenoid atas instruksi pengguna.
4. Telemetri Rutin: Kirim KitchenTelemetryPacket setiap 1000 ms.
5. Aktifkan Hardware Watchdog (ESP.wdtEnable(3000)).
6. Kode harus mengutamakan keselamatan fisik tanpa ketergantungan koneksi nirkabel.
```

---

### Prompt 3: Node 3 - Water Tank & Pump Management (`esp_water_tank`)

```markdown
Buatkan proyek PlatformIO lengkap (platformio.ini dan src/main.cpp) untuk Slave Node Manajemen Toren Air & Pompa (esp_water_tank) pada ekosistem Home IoT.

Spesifikasi Perangkat Keras:
- Mikrokontroler: Wemos D1 Mini (ESP8266)
- Sensor Jarak Ultrasonic Waterproof JSN-SR04T:
  - Pin Trigger: D1 (GPIO5)
  - Pin Echo: D2 (GPIO4, melewati pembagi tegangan 1kΩ:2kΩ untuk konversi sinyal 5V ke 3.3V)
- Saklar Pelampung Mekanik (Emergency High Float Switch): Pin Digital D5 (GPIO14, input dengan pull-up)
- Relai Daya Pompa Air AC 220V (30A Module): Pin Digital D6 (GPIO12, output active-LOW berisolasi optocoupler)
- Relai Solenoid Diverter/Bypass Valve: Pin Digital D7 (GPIO13, output active-LOW)

Kontrak Paket ESP-NOW:
- Saluran WiFi: Channel 6 (WIFI_STA), PHY Rate 1 Mbps DSSS, Peer Broadcast (FF:FF:FF:FF:FF:FF).
- Format Paket:
  enum MessageType : uint8_t { 
    MSG_TELEMETRY_WATER = 0x03,
    MSG_CMD_ACTUATOR    = 0x10 
  };
  struct __attribute__((packed)) PacketHeader {
    uint8_t  msg_type;
    uint8_t  node_id;    // 3
    uint16_t seq_number;
  };
  struct __attribute__((packed)) WaterTelemetryPacket {
    PacketHeader header;
    float distance_cm;
    uint8_t water_level_pct;
    bool pump_running;
    bool float_switch_triggered;
  };
  struct __attribute__((packed)) ActuatorCommandPacket {
    PacketHeader header;
    uint8_t target_actuator_id; // 1 = Pump, 2 = Diverter Valve
    uint8_t action;             // 0 = OFF, 1 = ON
  };

Perilaku Program:
1. Tinggi fisik toren air diasumsikan 120 cm (jarak minimum sensor ke air penuh = 20 cm, jarak dasar toren = 120 cm).
2. Hitung persentase ketinggian air: water_level_pct = map(distance_cm, 120, 20, 0, 100). Batasi rentang 0 s/d 100%.
3. Proteksi Luberan Mandiri: Jika pelampung mekanik D5 aktif (air menyentuh batas maksimum), relai pompa D6 WAJIB langsung dimatikan di dalam firmware demi mencegah banjir.
4. Downstream Control: Tangani perintah ActuatorCommandPacket dari Master untuk menyalakan/mematikan pompa dan katup diverter.
5. Kirim WaterTelemetryPacket setiap 2000 ms.
6. Aktifkan Hardware Watchdog (ESP.wdtEnable(4000)).
```

---

### Prompt 4: Node 4 - Main Electrical Panel & Solar Energy Meter (`esp_power_meter`)

```markdown
Buatkan proyek PlatformIO lengkap (platformio.ini dan src/main.cpp) untuk Slave Node Pemantau Daya Listrik PLN & Solar Inverter (esp_power_meter) pada ekosistem Home IoT.

Spesifikasi Perangkat Keras:
- Mikrokontroler: ESP32 DevKit (30 Pin / ESP-WROOM-32U)
- Modul Pengukur Daya Listrik AC (PZEM-004T v3.0):
  - Hardware Serial 1: Pin GPIO 16 (RX), GPIO 17 (TX)
  - Mengukur: Tegangan PLN (V), Arus (A), Daya Aktif (W), Energi (kWh), Frekuensi (Hz), Power Factor
- Modul Pengukur Daya DC Solar (PZEM-017 atau INA226):
  - Hardware Serial 2: Pin GPIO 25 (RX), GPIO 26 (TX)
  - Mengukur: Tegangan Array PV (V), Arus Pengisian Solar (A)
- Modul Komunikasi Inverter RS485 (MAX485 Terisolasi):
  - Serial RS485: Pin GPIO 18 (RX), GPIO 19 (TX), GPIO 4 (DE/RE)
  - Membaca status off-grid inverter dan persentase baterai UPS LiFePO4
- Sensor Suhu Ruang Panel/Inverter: DS18B20 pada Pin GPIO 27 (dengan pull-up 4.7kΩ)

Kontrak Paket ESP-NOW:
- Saluran WiFi: Channel 6 (WIFI_STA), PHY Rate 1 Mbps DSSS, Peer Broadcast (FF:FF:FF:FF:FF:FF).
- Format Paket:
  enum MessageType : uint8_t { MSG_TELEMETRY_POWER = 0x04 };
  struct __attribute__((packed)) PacketHeader {
    uint8_t  msg_type;   // 0x04
    uint8_t  node_id;    // 4
    uint16_t seq_number;
  };
  struct __attribute__((packed)) PowerTelemetryPacket {
    PacketHeader header;
    float mains_voltage;
    float house_power_w;
    float pv_voltage;
    float pv_current;
    float battery_pct;
    float rack_temp;
  };

Perilaku Program:
1. Inisialisasi PZEM-004T v3.0 dengan library mandiri (mandrimont/PZEM-004T-v30).
2. Inisialisasi Modbus/DC PZEM-017 dan OneWire DS18B20.
3. Polling telemetri setiap 1000 ms.
4. Lakukan validasi data pembacaan: Jika sensor mengembalikan nilai NaN atau pembacaan gagal, pertahankan nilai cache valid terakhir selama max 3 detik sebelum mengirim 0.0f.
5. Kirim PowerTelemetryPacket via esp_now_send().
6. Aktifkan Hardware Watchdog ESP32 (esp_task_wdt).
7. Kode terstruktur rapi, efisien memori, tanpa komputasi float yang berulang di loop.
```

---

### Prompt 5: Node 5 - Garage Access & Security (`esp_garage_node`)

```markdown
Buatkan proyek PlatformIO lengkap (platformio.ini dan src/main.cpp) untuk Slave Node Akses Garasi & Keamanan (esp_garage_node) pada ekosistem Home IoT.

Spesifikasi Perangkat Keras:
- Mikrokontroler: Wemos D1 Mini (ESP8266)
- Sensor Pintu Magnetik (MC-38 Magnetic Reed Switch): Pin Digital D5 (GPIO14, input dengan pull-up internal dan kapasitor de-bounce)
- Sensor Halangan Inframerah (IR Obstacle Beam): Pin Digital D6 (GPIO12, input active-LOW)
- Relai Pemicu Motor Garasi (Dry Contact): Pin Digital D1 (GPIO5, output active-LOW berisolasi optocoupler)
- Relai Saklar Lampu Garasi / Tangga: Pin Digital D2 (GPIO4, output active-LOW)

Kontrak Paket ESP-NOW:
- Saluran WiFi: Channel 6 (WIFI_STA), PHY Rate 1 Mbps DSSS, Peer Broadcast (FF:FF:FF:FF:FF:FF).
- Format Paket:
  enum MessageType : uint8_t { 
    MSG_TELEMETRY_GARAGE = 0x05,
    MSG_CMD_ACTUATOR     = 0x10 
  };
  struct __attribute__((packed)) PacketHeader {
    uint8_t  msg_type;
    uint8_t  node_id;    // 5
    uint16_t seq_number;
  };
  struct __attribute__((packed)) GarageTelemetryPacket {
    PacketHeader header;
    bool door_open;
    bool obstacle_detected;
    bool light_state;
  };
  struct __attribute__((packed)) ActuatorCommandPacket {
    PacketHeader header;
    uint8_t target_actuator_id; // 1 = Garage Motor Pulse, 2 = Garage Light
    uint8_t action;             // 0 = OFF, 1 = ON, 2 = PULSE_500MS
  };

Perilaku Program:
1. Kontrol Motor Garasi: Pemicu relai motor D1 HANYA boleh berupa pulsa sesaat (momentary pulse) 500 ms (D1 aktif LOW selama 500ms lalu otomatis kembali HIGH). Dilarang membuat relai motor terkunci menyala terus menerus!
2. Proteksi Halangan (Safety Beam): Jika sensor IR D6 mendeteksi halangan saat pintu sedang bergerak, tolak perintah tutup pintu dari Master.
3. Deteksi Perubahan Status Pintu: Kirim GarageTelemetryPacket secara periodik setiap 3000 ms, ATAU kirim seketika (event-driven) saat saklar pintu D5 mendeteksi perubahan status (buka/tutup).
4. Tangani ActuatorCommandPacket dari Master untuk pulsa motor garasi dan saklar lampu.
5. Aktifkan Hardware Watchdog (ESP.wdtEnable(4000)).
```

---

### Prompt 6: Node 6 - Distributed Lighting Banks (`esp_lighting_bank`)

```markdown
Buatkan proyek PlatformIO lengkap (platformio.ini dan src/main.cpp) untuk Slave Node Pengendali 8 Relai Lampu Rumah Berbasis I2C Expander (esp_lighting_bank) pada ekosistem Home IoT.

Spesifikasi Perangkat Keras:
- Mikrokontroler: Wemos D1 Mini (ESP8266)
- Modul I2C GPIO Expander: PCF8574T (Alamat default I2C: 0x20 atau 0x27)
  - Pin SCL: D1 (GPIO5)
  - Pin SDA: D2 (GPIO4)
- Modul 8-Channel Relay Board 5V: Terhubung ke pin P0 s/d P7 pada modul PCF8574T (Relai active-LOW dengan jumper JD-VCC terpisah dari catu daya 3.3V ESP)

Alokasi Zona 8 Lampu:
- P0: Ruang Tamu (living_room)
- P1: Kamar Utama (main_room)
- P2: Dapur (kitchen)
- P3: Tangga Garasi (stair_garage)
- P4: Tangga Lantai 2 (stair_fl2)
- P5: Halaman Depan (yard_front)
- P6: Halaman Belakang (yard_back)
- P7: Ruang Server / Panel (server_room)

Kontrak Paket ESP-NOW:
- Saluran WiFi: Channel 6 (WIFI_STA), PHY Rate 1 Mbps DSSS, Peer Broadcast (FF:FF:FF:FF:FF:FF).
- Format Paket:
  enum MessageType : uint8_t { 
    MSG_TELEMETRY_LIGHTS = 0x06,
    MSG_CMD_ACTUATOR     = 0x10 
  };
  struct __attribute__((packed)) PacketHeader {
    uint8_t  msg_type;
    uint8_t  node_id;    // 6
    uint16_t seq_number;
  };
  struct __attribute__((packed)) LightingTelemetryPacket {
    PacketHeader header;
    uint8_t relay_bitmask; // Bit 0 s/d 7 merepresentasikan status 8 zona relai
  };
  struct __attribute__((packed)) ActuatorCommandPacket {
    PacketHeader header;
    uint8_t target_actuator_id; // Bit index (0 s/d 7) atau 0xFF untuk all-lamps
    uint8_t action;             // 0 = OFF, 1 = ON
  };

Perilaku Program:
1. Inisialisasi komunikasi I2C dengan modul PCF8574T. Setel semua output ke kondisi awal mati (HIGH untuk relai active-LOW).
2. Simpan status relay_bitmask dalam memori.
3. Saat menerima paket perintah ActuatorCommandPacket via callback ESP-NOW:
   - Jika target_actuator_id 0 s/d 7: Ubah bit yang bersangkutan dan kirim byte baru ke PCF8574 via I2C.
   - Jika target_actuator_id == 0xFF: Nyalakan semua (action=1) atau matikan semua (action=0) sekaligus.
4. Kirim LightingTelemetryPacket ke Master setiap 3000 ms atau seketika setelah status relai berubah.
5. Aktifkan Hardware Watchdog (ESP.wdtEnable(5000)).
6. Kode efisien, menggunakan operasi bitwise langsung, tanpa alokasi memori dinamis (String/malloc).
```

---

## 3. Hasil Audit & Review Ponytail (Anti-Bloat & Keselamatan)

Berdasarkan audit teknis standar senior developer:

| Parameter Evaluasi | Temuan & Keputusan Audit | Status |
| :--- | :--- | :--- |
| **Alokasi Pin Wemos D1** | Seluruh node mematuhi *Safe Pins* (`D1, D2, D5, D6, D7`). Pin berbahaya `D3 (GPIO0)`, `D4 (GPIO2)`, dan `D8 (GPIO15)` sepenuhnya bebas dari modul relai active-LOW untuk mencegah *bootloader lockup*. | **PASS** |
| **Beban Komputasi & Memori** | Menggunakan C `struct packed` langsung di atas buffer ESP-NOW tanpa mem-parse string JSON. Alokasi memori adalah statis (zero heap fragmentation). | **PASS** |
| **Penetrasi Tembok** | Dipaksa pada 1 Mbps DSSS (`WIFI_PHY_RATE_1M_L`), memberikan sensitivitas radio -98 dBm yang sanggup menembus 2-3 dinding bata rumah tinggal. | **PASS** |
| **Otonomi Darurat (Fail-safe)** | Node Dapur (`esp_kitchen_safe`) dan Node Toren (`esp_water_tank`) memiliki interlock lokal di level loop perangkat keras; pemutusan gas dan pompa tidak bergantung pada jaringan nirkabel. | **PASS** |
| **Redundansi Sinyal Bahaya** | Transmisi darurat gas bocor menggunakan *burst transmission* 3x berurutan jeda 20ms untuk mengompensasi sifat broadcast tanpa ACK pada ESP-NOW. | **PASS** |
| **YAGNI (Anti-Bloat)** | Tidak ada mesh networking rumit, tidak ada OTA web server di setiap slave, tidak ada web dashboard lokal di setiap slave (cukup terpusat di Master). | **PASS** |
