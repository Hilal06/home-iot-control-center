# Layanan: Rust Edge Ingestion Gateway (`Service_Backend`)

Dokumen ini menjelaskan arsitektur, mekanisme konkurensi, alur data asinkron, dan pengoperasian **Rust Edge Ingestion Gateway**. Layanan ini beroperasi pada port `8080` (HTTP & WebSocket) dan bertindak sebagai gerbang terdepan (*first-line ingestion engine*) antara perangkat keras fisik (ESP32) dan lapisan backend tingkat atas.

---

## 1. Filosofi Desain & Alasan Penggunaan Rust

Dalam sistem IoT rumah tangga, paket telemetri sensor dialirkan setiap 1000 milidetik tanpa henti. Memilih Rust untuk lapisan ini didasarkan pada pertimbangan teknik perangkat keras:

1. **Nol Jeda Pembersihan Memori (*Zero Garbage Collection Jitter*)**:
   - Runtime dengan pelacak memori otomatis (*Garbage Collector*) seperti Node.js atau Java dapat menimbulkan jeda mikro (*stop-the-world GC pause*). Pada grafik dashboard real-time, jeda ini menyebabkan fluktuasi latensi (*latency jitter*).
   - Manajemen memori Rust berbasis sistem *ownership & borrowing* yang divalidasi saat kompilasi, menghasilkan eksekusi deterministik dengan latensi pemrosesan sub-milidetik (< 2 ms).
2. **Efisiensi Memori Tingkat Rendah (~15 MB RAM)**:
   - Service Rust hanya membutuhkan alokasi memori sekitar 15–25 MB saat menangani koneksi MQTT persisten dan puluhan klien WebSocket aktif, ideal dijalankan pada mini PC hemat daya (Intel N100 atau Raspberry Pi 4).
3. **Model Konkurensi Berbasis Task Tokio**:
   - Penanganan I/O nirkabel, soket jaringan, dan antrean data dijalankan secara non-blocking melalui pustaka `tokio` menggunakan mekanisme *epoll* kernel Linux tanpa membebani sistem operasi dengan ribuan thread berat.

---

## 2. Diagram Alur Data & Arsitektur Internal

Diagram berikut memvisualisasikan bagaimana satu paket data telemetri yang masuk dari MQTT dipecah (*fan-out*) secara paralel ke berbagai tujuan tanpa saling memblokir:

```mermaid
flowchart TD
    subgraph INGRESS["Lapisan Ingress Fisik"]
        ESP["ESP32 Master Node<br/>(ESP-NOW Aggregator)"] -->|"Publish MQTT (1000ms)<br/>home/sensors/telemetry"| MOSQ["Eclipse Mosquitto Broker<br/>Port: 1883"]
    end

    subgraph RUST_CORE["Rust Edge Ingestion Gateway (Port 8080)"]
        RUMQTT["rumqttc EventLoop Task<br/>(Asynchronous Stream Consumer)"]
        PARSE["serde_json Deserializer<br/>(Zero-copy parsing ke TelemetryPayload)"]
        
        subgraph FAN_OUT["Paralelisasi Aliran Data (Tokio Concurrency)"]
            CACHE["In-Memory RwLock Cache<br/>(State snapshot terbaru)"]
            WS_CH["tokio::sync::broadcast Channel<br/>(Buffer: 100 pesan)"]
            REDIS_PUB["Redis Tokio Multiplexed Conn<br/>(Publish: iot:telemetry:stream)"]
            DB_TASK["Tokio Spawn Worker<br/>(SQLx Pool Insertion)"]
        end
    end

    subgraph EGRESS["Lapisan Konsumen & Penyimpanan"]
        WS_CLIENTS["Browser Dashboard (Svelte 5)<br/>Endpoint: ws://host:8080/api/ws"]
        SPRING["Spring Boot Business Engine<br/>(Redis Stream Listener)"]
        TIMESCALE["TimescaleDB Hypertable<br/>(Indexed Time-Series Storage)"]
    end

    MOSQ -->|"TCP Stream"| RUMQTT
    RUMQTT --> PARSE
    PARSE --> CACHE
    PARSE --> WS_CH
    PARSE --> REDIS_PUB
    PARSE --> DB_TASK

    WS_CH -->|"WebSocket Push"| WS_CLIENTS
    REDIS_PUB -->|"Pub/Sub Channel"| SPRING
    DB_TASK -->|"Batch SQL Insert"| TIMESCALE
```

---

## 3. Komponen & Modul Utama

Arsitektur kode diatur dalam pola pemisahan tanggung jawab yang ringkas (*modular separation of concerns*):

```
Service_Backend/src/
├── main.rs              # Bootstrapping: inisialisasi pool, router Axum, watchdog task
├── config.rs            # Pemuat variabel lingkungan sistem (.env)
├── state.rs             # AppState global (DbPool, MqttClient, RedisClient, Broadcast channel)
├── api/
│   ├── routes.rs        # Registrasi rute HTTP REST & endpoint WebSocket
│   ├── ws.rs            # Handler koneksi WebSocket streaming
│   ├── handlers_control.rs # Handler eksekusi saklar dan kontrol aktuator
│   └── handlers_devices.rs # Handler status perangkat dan device lock
├── mqtt/
│   ├── client.rs        # Inisialisasi klien rumqttc ke broker Mosquitto
│   └── handler.rs       # Loop pemrosesan event: parsing telemetri, LWT, & Redis publisher
├── db/
│   ├── repository.rs    # Eksekusi kueri SQLx (TimescaleDB hypertable & migrasi)
│   └── mod.rs
├── fallback/
│   └── http_client.rs   # Klien direct-HTTP fallback ke IP ESP32
└── models/
    ├── telemetry.rs     # Struct representasi telemetri sensor (Serde)
    ├── command.rs       # Struct format instruksi kontrol aktuator
    └── device.rs        # Struct metadata inventaris perangkat
```

### 3.1. Penanganan Telemetri Asinkron (`mqtt/handler.rs`)
Saat pesan masuk pada topik `home/sensors/telemetry`:
1. Buffer biner dide-serialisasi langsung ke struct `TelemetryPayload`.
2. Menyimpan referensi snapshot terbaru ke dalam `tokio::sync::RwLock` untuk melayani panggilan REST `GET /api/devices/{id}/latest`.
3. Memancarkan payload ke channel broadcast WebSocket (`broadcast_tx.send()`). Klien browser menerima pembaruan instan tanpa jeda HTTP polling.
4. Mempublikasikan string JSON ke Redis Pub/Sub channel `iot:telemetry:stream` untuk dikonsumsi oleh lapisan analitik bisnis (Spring Boot).
5. Memicu task asinkron (`tokio::spawn`) untuk mencatat histori ke database TimescaleDB, memastikan latensi ingestion tidak tertahan oleh waktu tunggu I/O disk database.

### 3.2. Deteksi Perangkat Mati & Watchdog Otomatis (`main.rs`)
Sistem menjaga akurasi status perangkat dengan dua lapis deteksi:
1. **MQTT Last Will and Testament (LWT)**: Jika ESP32 terputus secara mendadak (kabel putus atau listrik padam), broker Mosquitto otomatis mempublikasikan pesan LWT ke `home/{device_id}/status` dengan status `OFFLINE`.
2. **Watchdog Background Interval**: Sebuah worker periodik berjalan setiap 3 detik memeriksa kolom `last_seen` di database. Jika suatu perangkat tidak mengirimkan telemetri selama lebih dari 5 detik, statusnya otomatis dialihkan menjadi `OFFLINE`:

```sql
UPDATE devices
SET status = 'OFFLINE'
WHERE status = 'ONLINE' AND (last_seen IS NULL OR last_seen < NOW() - INTERVAL '5 seconds');
```

### 3.3. Transparan Direct-HTTP Fallback (`fallback/http_client.rs`)
Saat dashboard atau backend mengirim perintah saklar (misal: mematikan lampu atau menutup katup gas):
- Gateway pertama kali mencoba mempublikasikan pesan ke broker MQTT (`home/actuators/control`).
- Jika broker MQTT sedang mengalami gangguan koneksi, handler secara otomatis mengalihkan perintah melalui koneksi langsung HTTP REST ke alamat IP lokal ESP32 (`POST http://<esp_ip>/api/control`).
- Hal ini menjamin pengguna tetap dapat mengendalikan rumah saat sistem broker sedang dalam pemeliharaan.

---

## 4. Konfigurasi Lingkungan (`.env`)

Layanan membaca parameter operasional melalui berkas lingkungan:

| Variabel | Nilai Default | Keterangan |
| :--- | :--- | :--- |
| `PORT` | `8080` | Port antarmuka HTTP REST dan WebSocket |
| `HOST` | `0.0.0.0` | Antarmuka jaringan binding service |
| `DATABASE_URL` | `postgres://iot_admin:iot_secret_password@localhost:5432/home_iot_db` | URL koneksi basis data TimescaleDB / PostgreSQL |
| `MQTT_BROKER_HOST` | `127.0.0.1` | Alamat host broker Eclipse Mosquitto |
| `MQTT_BROKER_PORT` | `1883` | Port broker MQTT |
| `REDIS_URL` | `redis://127.0.0.1:6379` | URL koneksi Redis Pub/Sub |
| `MQTT_TELEMETRY_TOPIC`| `home/sensors/telemetry` | Topik utama penerimaan telemetri sensor |
| `RUST_LOG` | `home_iot_backend=info,tower_http=debug` | Tingkat granulasi logging sistem |

---

## 5. Deployment & Multi-Stage Docker

Kompilasi container menggunakan arsitektur dua tahap (*multi-stage build*) untuk menghasilkan image akhir berukuran minimal (~45 MB) berbasis `debian:bookworm-slim` tanpa compiler toolchain:

```mermaid
flowchart LR
    STAGE1["Stage 1: Builder<br/>rust:1.88-slim<br/>(Kompilasi cargo build --release)"] -->|"Salin Biner Hasil Kompilasi<br/>(Ukuran ~18 MB)"| STAGE2["Stage 2: Runtime<br/>debian:bookworm-slim<br/>(Total Image: ~45 MB)"]
```

---

## 6. Referensi Resmi

1. **Tokio Runtime Architecture**: [https://tokio.rs/tokio/tutorial](https://tokio.rs/tokio/tutorial)
2. **Axum Web Framework Specification**: [https://docs.rs/axum/latest/axum/](https://docs.rs/axum/latest/axum/)
3. **MQTT v3.1.1 / v5.0 Protocol Standard (OASIS)**: [https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
4. **TimescaleDB Hypertable Documentation**: [https://docs.timescale.com/use-timescale/latest/hypertables/](https://docs.timescale.com/use-timescale/latest/hypertables/)
5. **Redis Pub/Sub Specifications**: [https://redis.io/docs/latest/develop/interact/pubsub/](https://redis.io/docs/latest/develop/interact/pubsub/)
