# FITALIS — Smart Medicine Reminder (v0.1.0)

Pengingat minum obat berbasis ESP32 DevKit V1. Punya dashboard web lokal buat
setting jadwal lewat HP — **tanpa internet, tanpa laptop, tanpa server**.

## Fitur v0.1.0

- Jadwal minum obat tersimpan di flash ESP32 (aman kalau listrik mati)
- Buzzer bunyi otomatis saat waktunya minum
- Tombol fisik buat konfirmasi "sudah minum"
- Dashboard web lokal: tambah/ubah/hapus jadwal, pantau status
- Alarm tetap jalan walau WiFi mati
- Notifikasi ke HP (opsional, via ntfy.sh atau Telegram)
- Jam dari NTP; bisa di-set manual dari dashboard kalau nggak ada internet

## Struktur file

```
fitalis-firmware/
├── FITALIS/
│   └── FITALIS.ino    <-- SATU FILE buat Arduino IDE (semua kode digabung)
├── include/
│   ├── config.h       <-- SEMUA setelan ada di sini (pin, WiFi, jadwal)
│   ├── schedule.h
│   ├── reminder.h
│   └── web.h
├── src/
│   ├── main.cpp       <-- setup() + loop()
│   ├── schedule.cpp   <-- simpan/baca jadwal dari flash
│   ├── reminder.cpp   <-- mesin alarm + jam
│   └── web.cpp        <-- web server + halaman dashboard
├── build_ino.py       <-- generate ulang FITALIS.ino dari src/ + include/
├── platformio.ini
└── .github/workflows/firmware.yml
```

`FITALIS.ino` di-generate otomatis dari `src/` + `include/`. Kalau lu ubah
kode di `src/`, jalankan `python3 build_ino.py` buat bikin ulang `.ino`-nya.

## Status build (terverifikasi)

Compile terakhir di GitHub Actions runner, ESP32 Dev Module, Arduino core:

```
RAM:   15.1%  (49,436 / 327,680 bytes)
Flash: 82.2%  (1,077,481 / 1,310,720 bytes)
firmware.bin: 1.1 MB
Build time: 91.76 s
Status: SUCCESS
```


## Rangkaian hardware

| Komponen | Pin ESP32 | Keterangan |
|---|---|---|
| Buzzer aktif (+) | GPIO 25 | (−) ke GND |
| Tombol konfirmasi | GPIO 27 | kaki lain ke GND |
| LED indikator | GPIO 2 | onboard, sudah ada |

Tombol pakai `INPUT_PULLUP`, jadi tidak perlu resistor tambahan.

## Cara upload ke ESP32

### Opsi A — Arduino IDE (paling gampang)

Buka file **`FITALIS/FITALIS.ino`** di Arduino IDE. Semua kode sudah digabung
jadi satu file, jadi tidak perlu copy-paste manual.

1. Install board ESP32: `File → Preferences → Additional Boards Manager URLs`
   → `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. `Tools → Board → Boards Manager` → cari **esp32** → install
3. Buka `FITALIS/FITALIS.ino`
4. Edit **BAGIAN 1 — KONFIGURASI**: isi `STA_SSID` dan `STA_PASSWORD`
5. `Tools → Board → ESP32 Arduino → ESP32 Dev Module`
6. Pilih port, klik **Upload**
7. Buka **Serial Monitor** di **115200 baud**

### Opsi B — PlatformIO (kalau sudah pakai VS Code)

Buka folder project ini di VS Code dengan extension PlatformIO, lalu klik
**Upload**. Pakai `include/config.h` (bukan file `.ino`).


## Cara pakai

1. Setelah upload, Serial Monitor akan menampilkan IP address:
   ```
   Dashboard : http://192.168.1.42/
   ```
2. Buka IP itu di browser HP (HP harus tersambung ke WiFi yang sama)
3. Tab **Jadwal** → isi nama obat, dosis, jam, hari → **Simpan**
4. Selesai. Buzzer akan bunyi otomatis di jam itu.

### Mode hotspot (tanpa router)

Kalau nggak ada WiFi sama sekali, ubah di `config.h`:

```c
#define WIFI_MODE   MODE_AP
```

ESP32 jadi hotspot sendiri: SSID **FITALIS**, password `fitalis123`.
Buka `http://192.168.4.1`. Tapi NTP nggak jalan — set jam manual di
tab **Info**.

## Notifikasi ke HP (opsional)

Default **mati**. Untuk mengaktifkan, di `config.h`:

```c
#define NOTIFY_ENABLED 1
#define NOTIFY_URL "http://ntfy.sh/fitalis-obat-kevin-9x2"
```

Lalu install app **ntfy** di HP, subscribe ke channel dengan nama sama.
Atau pakai Telegram:

```c
#define NOTIFY_URL "https://api.telegram.org/bot<TOKEN>/sendMessage?chat_id=<ID>&text={msg}"
```

Notifikasi hanya dikirim sekali per alarm. Alarm tetap berbunyi walau
notifikasi gagal.

## Yang belum ada (rencana berikutnya)

- Layar TFT ST7735S (tampilkan jam + nama obat)
- Sensor IR per wadah obat (deteksi obat diambil)
- Log riwayat minum
- GPS NEO-6M + pencarian apotek terdekat
- BPM monitoring
- Backend Python + database

## Catatan

Firmware ini bisa di-compile dan diverifikasi otomatis lewat GitHub Actions
(`.github/workflows/firmware.yml`). Push ke repo → Actions build → download
`firmware.bin` dari tab Artifacts. Ini cara paling praktis kalau nggak mau
install Arduino IDE.
