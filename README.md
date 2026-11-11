# Home IoT Control Center

[![Architecture: Hybrid Rust + Java](https://img.shields.io/badge/Architecture-Hybrid%20Rust%20%2B%20Java-orange.svg)](#architecture-overview)
[![Dashboard: Svelte 5 SCADA](https://img.shields.io/badge/Dashboard-Svelte%205%20SCADA-blue.svg)](#frontend-dashboard)
[![Edge: ESP32 + PlatformIO](https://img.shields.io/badge/Edge-ESP32%20PlatformIO-green.svg)](#embedded-firmware)
[![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)](#license)

**Home IoT Control Center** adalah platform otomasi dan manajemen energi rumah pintar berarsitektur **Hybrid Microservice**. Sistem ini dirancang untuk menyatukan penanganan I/O fisik berkecapatan tinggi (*sub-millisecond edge streaming*) dengan logika otomasi cerdas (*enterprise safety rules, dynamic scenes, and PLN energy accounting*), divisualisasikan dalam antarmuka SCADA industrial yang ringkas dan responsif.

---

## 🏛️ Arsitektur Monorepo

```
home-iot-control-center/
├── .env.example               # Template environment variables publik
├── docker-compose.yml         # Orkestrasi multi-service lokal & production
├── AGENTS.md                  # Project rules (Ponytail & Impeccable)
├── Devices/
│   └── ESP_Master/            # Firmware ESP32 (PlatformIO C++) dengan injeksi dinamis .env
├── Service_Backend/           # Edge Ingestion Gateway (Rust + Axum + TimescaleDB + Redis)
├── Service_Business_Java/     # Business Engine (Spring Boot 3.3 + Java 17 Records)
├── Dashboard/                 # SCADA Industrial Dashboard (Svelte 5 + Tailwind CSS)
└── docs/                      # Dokumentasi teknis & spesifikasi kontrak
    ├── architecture/          # Rancangan arsitektur hybrid & benchmark
    ├── services/              # Panduan internal Rust & Spring Boot
    ├── contracts/             # JSON schemas, Redis channels, & REST endpoints
    └── guides/                # Panduan setup DuckDNS & firmware ESP32
```

---

## ⚡ Fitur Utama

### 1. Ingestion Gateway Berlatensi Rendah (Rust + Axum)
- **Ultra-low Footprint**: Mengonsumsi ~15MB RAM saat menangani stream MQTT dan puluhan koneksi WebSocket.
- **TimescaleDB Time-Series Engine**: Menyimpan telemetri listrik, suhu, gas MQ, dan panel surya ke hypertable terindeks.
- **Bi-directional Redis Pub/Sub**: Meneruskan stream telemetri mentah ke Redis channel `iot:telemetry:stream` dan menerima sinyal eksekusi scene dari channel `iot:scene:events`.
- **Transparent Direct-HTTP Fallback**: Otomatis mengalihkan perintah ke IP lokal ESP32 (`/api/control`) jika broker MQTT sedang offline.

### 2. Business Engine & Safety Interlock (Spring Boot 3.3)
- **PLN 900W Overload Protection**: Memantau kapasitas beban secara real-time dan secara otomatis mematikan beban sekunder sebelum pemutus MCB PLN trip.
- **Hazard Interlock**: Menutup relay dan memicu sirene alarm secara instan saat kebocoran gas MQ atau sensor air aktif.
- **PLN Prepaid Token & Burn-Rate Tracker**: Menghitung sisa pulsa listrik PLN prabayar (Rp & kWh), laju konsumsi harian (*burn-rate*), dan estimasi hari tersisa hingga token habis.
- **Dynamic Scene Engine**: Presets 1-klik untuk seluruh rumah (`ALL_OFF`, `DAY_MODE`, `NIGHT_MODE`, `SOLAR_PRIORITY`).
- **Device Registry**: Melacak metadata perangkat, nama alias, lokasi ruangan, dan status konektivitas *live*.
- **Multi-Channel Alerting**: Pengiriman peringatan kritis melalui bot Telegram dan Discord Webhook.

### 3. Industrial SCADA Dashboard (Svelte 5)
- **Impeccable SCADA Aesthetic**: Desain rectilinear 0px corners, palet warna *dark slate* dengan aksen *amber & telemetry cyan*, bebas elemen visual berlebih (*anti-slop*).
- **Live Stream via WebSocket**: Pembaruan status instan tanpa polling HTTP.
- **Scene Selector Bar**: Pilihan mode rumah langsung di panel navigasi atas.
- **Widget Pulsa Listrik PLN**: Indikator persentase sisa kuota token listrik dengan estimasi sisa hari.

### 4. Firmware Node Cerdas (ESP32 Master)
- **Automated Credential Decoupling**: Build script `load_env.py` menyuntikkan SSID, password WiFi, dan IP MQTT dari file root `.env` ke preprocessor C++ secara otomatis saat kompilasi.
- **Fail-safe Direct HTTP Server**: Menyediakan REST API internal pada port 80 jika komunikasi MQTT terganggu.

---

## 🚀 Quickstart

### Prasyarat
- [Docker](https://www.docker.com/) & [Docker Compose](https://docs.docker.com/compose/) v2+
- (Opsional untuk development lokal): Rust 1.80+, JDK 17+, Node.js 20+, PlatformIO Core.

### Menjalankan Stack Lengkap
1. **Clone repositori:**
   ```bash
   git clone git@github.com:Hilal06/home-iot-control-center.git
   cd home-iot-control-center
   ```

2. **Siapkan environment variables:**
   ```bash
   cp .env.example .env
   # Buka .env dan sesuaikan kredensial WiFi, MQTT, dan database lokal Anda
   ```

3. **Jalankan container dengan Docker Compose:**
   ```bash
   docker compose up -d
   ```

4. **Akses antarmuka layanan:**
   - **SCADA Dashboard**: [http://localhost:5173](http://localhost:5173)
   - **Rust Ingestion Gateway**: [http://localhost:3000](http://localhost:3000)
   - **Spring Boot Business API**: [http://localhost:8081](http://localhost:8081)
   - **Eclipse Mosquitto (MQTT)**: `localhost:1883`

---

## 🔒 Privasi & Keamanan

File sensitif berikut **DIKECUALIKAN SECARA KETAT** dari repositori Git melalui `.gitignore`:
- `.env` (berisi password WiFi lokal, token bot, dan kredensial database).
- `.agents/` (konfigurasi MCP dan token asisten).
- `Devices/ESP_Master/include/secrets.h` (fallback kredensial firmware).
- Direktori build & dependencies (`target/`, `build/`, `node_modules/`, `.pio/`).

Gunakan file `.env.example` sebagai referensi saat melakukan deployment pada node server baru.

---

## 📚 Dokumentasi Lengkap

- [Rancangan Arsitektur Hybrid](docs/architecture/hybrid-overview.md)
- [Topologi ESP-NOW Master-Slave & Spesifikasi Node](docs/architecture/espnow-master-slave-topology.md)
- [Rust Edge Ingestion Service](docs/services/rust-edge-ingestion.md)
- [Spring Boot Business Engine](docs/services/springboot-business-engine.md)
- [Spesifikasi Kontrak & API Reference](docs/contracts/events-and-apis.md)
- [Panduan Setup Kredensial ESP32](docs/guides/esp32-environment-setup.md)
- [Panduan Akses Jarak Jauh DuckDNS](docs/guides/duckdns-remote-access.md)

---

## 📄 Lisensi

Proyek ini dilisensikan di bawah lisensi [MIT](LICENSE).

