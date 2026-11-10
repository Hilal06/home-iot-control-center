# Layanan: Rust Edge Ingestion Gateway (`Service_Backend`)

Dokumen teknis ini menjelaskan arsitektur, modul internal, alur data, konfigurasi, dan pengoperasian **Rust Edge Ingestion Gateway** pada port `8080`.

---

## 1. Peran & Tanggung Jawab Layanan

Layanan Rust bertindak sebagai garis terdepan (*first-line gateway*) yang berinteraksi langsung dengan perangkat keras fisik dan browser klien:

1. **MQTT Telemetry Ingestion**: Berlangganan topik `home/sensors/telemetry`, `home/+/telemetry`, dan `home/sensors/+/telemetry` untuk memproses paket data telemetri yang masuk setiap 1000ms via `rumqttc`.
2. **WebSocket Live Streaming**: Menyediakan endpoint `ws://<host>:8080/api/ws` berbasis `tokio::sync::broadcast` untuk meneruskan telemetri langsung ke Dashboard tanpa jeda polling.
3. **Hardware Watchdog & LWT Monitoring**:
   - Mendengarkan topik LWT broker `home/+/status`.
   - Menjalankan background task setiap 3 detik untuk mendeteksi perangkat fisik yang mati (`last_seen > 5 detik`).
4. **Redis Event Forwarding**: Menerbitkan paket telemetri dan status perangkat yang telah divalidasi ke channel Redis `iot:telemetry:stream` dan `iot:device:status`.
5. **Direct Actuator Execution & Fallback**: Meneruskan perintah saklar ke broker MQTT, dengan kemampuan *direct HTTP fallback* ke IP lokal ESP32 jika broker terganggu.

---

## 2. Struktur Modul & Sumber Kode

```
Service_Backend/src/
├── main.rs              # Entrypoint: konfigurasi, inisialisasi DB, spawn task watchdog & MQTT, Axum router
├── config.rs            # Parser variabel lingkungan (.env)
├── state.rs             # AppState global (DbPool, MqttClient, RedisClient, Broadcast channel)
├── api/
│   ├── routes.rs        # Pendaftaran rute HTTP & WebSocket Axum
│   ├── ws.rs            # Handler WebSocket streaming live
│   ├── handlers_control.rs # Handler kontrol saklar/aktuator (REST API)
│   └── handlers_devices.rs # Handler manajemen perangkat & lock
├── mqtt/
│   ├── client.rs        # Inisialisasi koneksi rumqttc ke broker Mosquitto
│   └── handler.rs       # Event loop MQTT: parser telemetri, LWT, & Redis publisher
├── db/
│   ├── repository.rs    # Query SQLx (PostgreSQL / TimescaleDB) & auto-migrations
│   └── mod.rs
└── models/
    ├── telemetry.rs     # Struct deserialisasi data sensor (Serde)
    ├── command.rs       # Struct format kontrol aktuator
    └── device.rs        # Struct representasi metadata perangkat
```

---

## 3. Detail Implementasi Teknis

<details>
<summary><strong>1. Penanganan Non-Blocking MQTT & Redis Publisher (`mqtt/handler.rs`)</strong> (Klik untuk melihat)</summary>

Event loop `rumqttc` berjalan di dalam task Tokio terpisah:
```rust
// Cuplikan logika publikasi ke Redis & WebSocket
match serde_json::from_slice::<TelemetryPayload>(&publish.payload) {
    Ok(mut telem) => {
        telem.device_id = Some(device_id.clone());

        // 1. Simpan snapshot terbaru di memory lock
        {
            let mut lock = state.latest_telemetry.write().await;
            *lock = Some(telem.clone());
        }

        // 2. Broadcast ke browser yang terhubung via WebSocket
        let _ = state.broadcast_tx.send(telem.clone());

        // 3. Teruskan ke Redis Pub/Sub untuk dikonsumsi Spring Boot
        if let Ok(json_str) = serde_json::to_string(&telem) {
            let mut redis_conn = state.redis_client.get_multiplexed_tokio_connection().await;
            let _ = redis_conn.publish::<_, _, ()>("iot:telemetry:stream", json_str).await;
        }

        // 4. Persistensi ke TimescaleDB secara asinkron
        let db_pool = state.db.clone();
        tokio::spawn(async move {
            let _ = save_telemetry(&db_pool, &device_id, &telem).await;
        });
    }
    Err(e) => {
        warn!("Gagal mendeserialisasi JSON telemetri: {:?}", e);
    }
}
```

</details>

<details>
<summary><strong>2. Hardware Watchdog Task (`main.rs`)</strong> (Klik untuk melihat)</summary>

Background worker memeriksa status perangkat setiap 3 detik. Jika tidak ada sinyal `> 5 detik`, status di database dan cache langsung diubah menjadi `OFFLINE`:
```rust
tokio::spawn(async move {
    let mut interval = tokio::time::interval(Duration::from_secs(3));
    loop {
        interval.tick().await;
        let _ = sqlx::query(
            r#"
            UPDATE devices
            SET status = 'OFFLINE'
            WHERE status = 'ONLINE' AND (last_seen IS NULL OR last_seen < NOW() - INTERVAL '5 seconds')
            "#
        )
        .execute(&watchdog_db)
        .await;
    }
});
```

</details>

---

## 4. Variabel Lingkungan (Configuration)

| Variabel | Nilai Default | Deskripsi |
| :--- | :--- | :--- |
| `PORT` | `8080` | Port HTTP & WebSocket service Rust |
| `HOST` | `0.0.0.0` | IP binding jaringan |
| `DATABASE_URL` | `postgres://iot_admin:iot_secret_password@localhost:5432/home_iot_db` | URL koneksi PostgreSQL/TimescaleDB |
| `MQTT_BROKER_HOST` | `127.0.0.1` | Alamat host broker Eclipse Mosquitto |
| `MQTT_BROKER_PORT` | `1883` | Port broker MQTT |
| `REDIS_URL` | `redis://127.0.0.1:6379` | URL koneksi Redis Pub/Sub |
| `MQTT_TELEMETRY_TOPIC`| `home/sensors/telemetry` | Topik utama telemetri sensor |
| `RUST_LOG` | `home_iot_backend=info,tower_http=debug` | Level log sistem |

---

## 5. Deployment dengan Docker

Service ini menggunakan *multi-stage Dockerfile* untuk menghasilkan image ramping berbasis `debian:bookworm-slim` (~45 MB total image size):

```dockerfile
# STAGE 1: Builder
FROM rust:1.88-slim AS builder
WORKDIR /app
COPY Cargo.toml Cargo.lock ./
COPY src ./src
RUN cargo build --release

# STAGE 2: Minimal Runtime
FROM debian:bookworm-slim AS runtime
WORKDIR /app
COPY --from=builder /app/target/release/home-iot-backend /usr/local/bin/home-iot-backend
EXPOSE 8080
ENTRYPOINT ["home-iot-backend"]
```

---

## 6. Referensi Dokumentasi Resmi

1. **Rust Language Reference**: [https://doc.rust-lang.org/reference/](https://doc.rust-lang.org/reference/)
2. **Tokio Async Engine**: [https://docs.rs/tokio/latest/tokio/](https://docs.rs/tokio/latest/tokio/)
3. **Axum Web Framework**: [https://docs.rs/axum/latest/axum/](https://docs.rs/axum/latest/axum/)
4. **Rumqttc Client**: [https://docs.rs/rumqttc/latest/rumqttc/](https://docs.rs/rumqttc/latest/rumqttc/)
5. **Redis-rs Tokio Client**: [https://docs.rs/redis/latest/redis/aio/index.html](https://docs.rs/redis/latest/redis/aio/index.html)

