# Home IoT Backend Service (Rust + Axum + rumqttc)

Service Backend performa tinggi (~10MB RAM) berbasis **Rust** untuk mengelola perangkat pada ekosistem Home IoT, khususnya **ESP32 Master Node** (`Devices/ESP_Master`) serta perangkat pendukung di masa mendatang.

---

## 1. Fitur Utama

- **MQTT Telemetry Ingestion**: Mengonsumsi stream telemetri sensor lingkungan, kelistrikan rumah, UPS, dan solar PV secara real-time dari topik `home/sensors/telemetry`.
- **TimescaleDB / PostgreSQL Time-Series Storage**: Menyimpan setiap snapshot pembacaan telemetri ke dalam hypertable/tabel terindeks untuk analitik data dan grafik historis.
- **Actuator Command & Control**: Menyediakan REST API untuk mengontrol 11 zona saklar lampu, pintu garasi, pemutus arus dapur, AC/HVAC, dan inverter solar melalui topik `home/actuators/control`.
- **Direct HTTP Fallback**: Jika broker MQTT sedang offline, perintah otomatis dialihkan secara transparan melalui direct REST HTTP ke IP lokal ESP32 (`/api/control`).
- **Real-Time WebSocket Streaming**: Streaming telemetri langsung ke browser atau aplikasi mobile via WebSocket (`/api/ws`) tanpa polling berulang.
- **Docker-First Architecture with Dual Networks**: Terintegrasi penuh bersama **Eclipse Mosquitto** dan **TimescaleDB** dalam satu berkas `docker-compose.yml`, dengan segmentasi jaringan terisolasi (`iot_public_net` & `iot_internal_net`).

---

## 2. Quickstart dengan Docker Compose

Jalankan seluruh stack (Mosquitto MQTT Broker + TimescaleDB + Backend Service) hanya dengan satu perintah:

```bash
cd Service_Backend
docker compose up -d
```

Periksa status container:
```bash
docker compose ps
```

Melihat log backend:
```bash
docker compose logs -f service_backend
```

Menghentikan seluruh layanan:
```bash
docker compose down
```

---

## 3. Menjalankan Secara Lokal (Tanpa Docker Backend)

Jika ingin menjalankan backend langsung di host (menggunakan compiler Rust lokal):

1. Pastikan PostgreSQL dan Mosquitto sudah berjalan (bisa menggunakan `docker compose up -d mosquitto timescaledb`).
2. Salin environment:
   ```bash
   cp .env.example .env
   ```
3. Jalankan backend:
   ```bash
   cargo run --release
   ```
4. Menjalankan automated test suite:
   ```bash
   cargo test
   ```

---

## 4. Referensi REST API

Base URL: `http://localhost:8080`

### 4.1 Health Check
- **Endpoint**: `GET /health`
- **Contoh Respon**:
  ```json
  {
    "status": "healthy",
    "service": "Home-IOT-Backend",
    "uptime_seconds": 45,
    "mqtt_broker": "mosquitto:1883",
    "telemetry_topic": "home/sensors/telemetry",
    "command_topic": "home/actuators/control"
  }
  ```

### 4.2 Daftar Perangkat Terdaftar
- **Endpoint**: `GET /api/devices`

### 4.3 Snapshot Status Perangkat
- **Endpoint**: `GET /api/devices/:id/status` (contoh: `/api/devices/esp32_master/status`)
- Mengembalikan konfigurasi perangkat, status 11 zona lampu, garasi, HVAC, serta snapshot telemetri terakhir.

### 4.4 Riwayat Telemetri (Time-Series)
- **Endpoint**: `GET /api/devices/:id/telemetry/history?limit=50`
- Mengambil array data metrik sensor berurutan dari waktu terbaru ke terlama.

### 4.5 Riwayat Aksi Aktuator (Audit Log)
- **Endpoint**: `GET /api/devices/:id/events?limit=50`
- Mengembalikan log riwayat perintah kontrol yang pernah dieksekusi beserta channel pengiriman (`MQTT` atau `HTTP_FALLBACK`).

---

## 5. Mengirim Perintah Kontrol Aktuator (`POST /api/devices/:id/control`)

Semua perintah kendali divalidasi terhadap batasan firmware ESP32 Master sebelum diterbitkan.

### A. Kontrol Saklar Lampu
Nyalakan/matikan lampu pada zona tertentu:
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_lamp", "room": "yard_front", "state": true}'
```
*Daftar nilai `room`: `living_room`, `main_room`, `garage`, `kitchen`, `stair_garage`, `stair_fl2`, `yard_front`, `yard_side`, `yard_back`, `server_room`, `electrical_room`, `all`.*

Master Switch (matikan semua lampu sekaligus):
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_lamp", "room": "all", "state": false}'
```

### B. Kontrol Pintu Garasi
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_garage", "state": "open"}'
```

### C. Kontrol Pemutus Arus Dapur (Kitchen Breaker)
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_breaker", "state": true}'
```

### D. Kontrol HVAC / AC Ruangan
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_hvac", "power": true, "target_temp": 22}'
```
*(Validasi: `target_temp` 16 s/d 32 Celsius)*

### E. Kontrol Solar Charger & Inverter
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_solar", "charge_enabled": true, "max_watts": 3500}'
```
*(Validasi: `max_watts` 100 s/d 15000 Watt)*

### F. Ubah Interval Telemetri ESP32
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "set_config", "telemetry_interval_ms": 3000}'
```

### G. Reboot Modul ESP32
```bash
curl -X POST http://localhost:8080/api/devices/esp32_master/control \
  -H "Content-Type: application/json" \
  -d '{"command": "reboot_esp"}'
```

---

## 6. Real-time WebSocket Stream

Klien web dashboard dapat langsung membuka koneksi WebSocket ke:
```
ws://localhost:8080/api/ws
```
- Setiap pesan telemetri baru yang masuk dari ESP32 Master akan langsung di-push ke semua klien yang terhubung secara instan.
- Klien yang baru terkoneksi akan langsung menerima snapshot status telemetri terakhir.
