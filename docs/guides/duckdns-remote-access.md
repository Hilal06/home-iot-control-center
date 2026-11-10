# Panduan Akses Jarak Jauh dengan DuckDNS & Docker

Dokumen ini berisi panduan setup Dynamic DNS (DDNS) gratis menggunakan **DuckDNS** untuk mengakses instance Home Assistant / IoT Service dari luar jaringan rumah tanpa membeli domain.

---

## 1. Konsep Dasar

* **DuckDNS** adalah layanan DDNS gratis yang memberikan subdomain berakhiran `*.duckdns.org` (contoh: `rumahku-iot.duckdns.org`).
* Anda **tidak perlu membeli domain**. DuckDNS menyediakan domain dan DNS resolver gratis seumur hidup.
* Container DuckDNS di Docker bertugas memantau IP publik router Anda. Setiap kali ISP mengubah IP publik (dynamic IP), container otomatis memperbarui DNS DuckDNS.

---

## 2. Cara Mendapatkan Token & Subdomain

1. Buka situs [duckdns.org](https://www.duckdns.org).
2. Login langsung menggunakan OAuth (klik **Sign in with Google** atau **GitHub**). Tidak perlu formulir registrasi.
3. Setelah login, Anda akan melihat token akun di bagian atas:
   ```text
   your account token is: a1b2c3d4-e5f6-7890-abcd-ef1234567890
   ```
4. Di bagian **domains**:
   - Masukkan nama subdomain yang diinginkan pada kolom input (misal: `rumahku-iot`).
   - Klik **add domain**.
   - Subdomain Anda sekarang aktif: `rumahku-iot.duckdns.org`.

---

## 3. Konfigurasi Docker (`docker-compose.yml`)

Tambahkan service `iot_duckdns` ke dalam `docker-compose.yml` (menggunakan image resmi LinuxServer):

```yaml
  iot_duckdns:
    image: lscr.io/linuxserver/duckdns:latest
    container_name: iot_duckdns
    restart: unless-stopped
    environment:
      - PUID=1000
      - PGID=1000
      - TZ=Asia/Jakarta
      - SUBDOMAINS=rumahku-iot             # Nama subdomain tanpa '.duckdns.org'
      - TOKEN=a1b2c3d4-e5f6-xxxx-xxxx      # Token dari akun DuckDNS
      - LOG_FILE=false
```

Jalankan container:
```bash
docker compose up -d iot_duckdns
```

Periksa log untuk memastikan sinkronisasi IP berhasil:
```bash
docker logs -f iot_duckdns
# Output sukses: Your IP was updated at ...
```

---

## 4. Konfigurasi Router (Port Forwarding)

Agar traffic dari internet bisa masuk ke Home Assistant atau IoT Dashboard:

1. Buka dashboard modem/router ISP Anda (biasanya `192.168.1.1` atau `192.168.100.1`).
2. Masuk ke menu **Port Forwarding** / **Virtual Server**.
3. Buat aturan forwarding baru:
   - **Service Name**: HomeAssistant
   - **External Port**: `8123` (atau port SSL `443`)
   - **Internal IP**: IP lokal server Linux Anda (misal: `192.168.1.50`)
   - **Internal Port**: `8123`
   - **Protocol**: TCP

> **Catatan CGNAT (Carrier-Grade NAT)**:
> Jika IP WAN di router Anda berada di rentang `100.64.0.0/10` atau `10.x.x.x`, berarti ISP Anda menggunakan CGNAT (Port Forwarding tidak akan tembus).
> Solusinya: Hubungi ISP untuk meminta IP publik dinamis, atau gunakan alternatif tunnel seperti **Tailscale** / **Cloudflare Tunnel**.

---

## 5. SSL / HTTPS (Let's Encrypt)

Google Assistant dan browser modern mewajibkan enkripsi HTTPS:
* Jika menggunakan **Home Assistant OS / Supervised**: Gunakan add-on resmi **DuckDNS** (sudah mencakup otomatisasi Let's Encrypt SSL).
* Jika menggunakan Docker Container standalone: Disarankan memasang reverse proxy seperti **Nginx Proxy Manager** (NPM) atau **Caddy** di depan Home Assistant untuk auto-renewal sertifikat SSL Let's Encrypt.

