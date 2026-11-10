# Panduan Konfigurasi Kredensial Firmware ESP32 & Environment Setup

Dokumen ini menjelaskan arsitektur pemisahan kredensial (*credential decoupling*) pada firmware ESP32 Master Node (`Devices/ESP_Master`) agar informasi sensitif seperti SSID WiFi, password, dan IP Broker MQTT tidak bocor ke Git repository.

---

## 1. Arsitektur Pemisahan Kredensial

Alih-alih menuliskan password WiFi langsung di dalam kode C++ (`main.cpp`), sistem menggunakan dua lapis proteksi:
1. **Otomasi Build PlatformIO (`load_env.py`)**: Script pre-build membaca file `.env` di root direktori proyek dan menginjeksi kredensial sebagai macro C++ preprocessor pada saat kompilasi.
2. **Fallback Header (`secrets.h`)**: Digunakan jika kompilasi dilakukan secara mandiri atau pada IDE yang tidak mengeksekusi script PlatformIO Python.

```
Root Project
├── .env                  <-- Ignored oleh Git (berisi sandi asli)
├── .env.example          <-- Template publik tanpa rahasia
└── Devices/ESP_Master/
    ├── load_env.py       <-- Script Python pre-build
    ├── platformio.ini    <-- Menjalankan pre:load_env.py
    ├── include/
    │   ├── secrets.h.example <-- Template fallback
    │   └── secrets.h         <-- Ignored oleh Git
    └── src/
        └── main.cpp      <-- Menggunakan macro C++ dinamis
```

---

## 2. Cara Kerja `load_env.py`

Saat menjalankan perintah build:
```bash
pio run
```

PlatformIO akan memicu script pre-build sebelum kompilasi C++ dimulai:
1. `load_env.py` mencari file `.env` pada root direktori: `../../.env`.
2. Script membaca variabel berikut:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `MQTT_BROKER_IP`
   - `MQTT_PORT`
3. Menambahkan build flags preprocessor ke dalam environment build PlatformIO:
   - `-DCONFIG_WIFI_SSID=\"...\"`
   - `-DCONFIG_WIFI_PASSWORD=\"...\"`
   - `-DCONFIG_MQTT_BROKER_IP=\"...\"`
   - `-DCONFIG_MQTT_PORT=1883`

---

## 3. Implementasi pada C++ (`src/main.cpp`)

Di dalam kode C++, macro preprocessor diperiksa dengan `#ifndef`:

```cpp
// Cek apakah build flag sudah disuntikkan dari .env via load_env.py
#ifndef CONFIG_WIFI_SSID
  #if __has_include("secrets.h")
    #include "secrets.h"
  #else
    #define CONFIG_WIFI_SSID "FALLBACK_SSID"
    #define CONFIG_WIFI_PASSWORD "FALLBACK_PASS"
    #define CONFIG_MQTT_BROKER_IP "192.168.1.100"
    #define CONFIG_MQTT_PORT 1883
  #endif
#endif
```

Variabel tersebut kemudian langsung digunakan saat inisialisasi koneksi:
```cpp
const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASSWORD;
const char* mqtt_server = CONFIG_MQTT_BROKER_IP;
const int   mqtt_port = CONFIG_MQTT_PORT;
```

---

## 4. Langkah Setup untuk Pengembang Baru

1. **Salin file template `.env.example` ke `.env` di root repository:**
   ```bash
   cp .env.example .env
   ```
2. **Sesuaikan kredensial jaringan lokal pada `.env`:**
   ```ini
   WIFI_SSID="Nama_WiFi_Rumah"
   WIFI_PASSWORD="Password_WiFi_Anda"
   MQTT_BROKER_IP="192.168.1.100"
   MQTT_PORT=1883
   ```
3. **Kompilasi dan Flash Firmware:**
   ```bash
   cd Devices/ESP_Master
   pio run -t upload
   ```
   *Output kompilasi akan menampilkan:*
   ```text
   [load_env.py] Injected ESP32 WiFi & MQTT credentials from /path/to/.env
   ```

---

## 5. Keamanan Git

File berikut telah didaftarkan dalam `.gitignore` di root monorepo dan **TIDAK AKAN PERNAH** terunggah ke repositori GitHub:
- `.env`
- `*.env`
- `Devices/ESP_Master/include/secrets.h`
- `.pio/` (artefak kompilasi PlatformIO)
