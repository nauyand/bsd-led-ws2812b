# BSD LED WS2812B

Indikator **Blind Spot Detection (BSD)** untuk mobil, berbasis **Arduino Mega** dan **16 LED WS2812B**.
Makin dekat objek di sisi kiri/kanan, makin banyak LED yang menyala dengan gradasi warna
**hijau → kuning → merah**, dan buzzer berbunyi makin cepat.

Program punya **mode simulasi** bawaan, jadi bisa dicoba tanpa sensor jarak: Arduino
membuat sendiri objek yang mendekat lalu menjauh secara acak.

## Fitur

- 16 LED dibagi **8 kiri** (LED 0 → 7) dan **8 kanan** (LED 15 → 8).
- Gradasi warna hijau → kuning → merah sesuai kedekatan objek.
- Animasi LED berjalan (bertambah/berkurang 1 LED tiap 25 ms), non-blocking.
- Buzzer kiri dan kanan terpisah, dengan pola bip mengikuti jarak (Timer1).
- Input jarak lewat Serial (`kiri,0,kanan,0`).
- Mode simulasi acak: sisi, kecepatan, dan jarak terdekat dipilih acak tiap ronde.

## Hardware

| Komponen | Keterangan |
|---|---|
| Arduino Mega 2560 | Mikrokontroler |
| LED WS2812B × 16 | Strip/ring addressable RGB |
| Buzzer aktif × 2 | Kiri dan kanan (opsional) |
| Resistor 220–470 Ω | Di jalur data LED (disarankan 330 Ω) |
| Kapasitor 470–1000 µF | Opsional, di antara 5V dan GND LED |

### Wiring

```
Mega 5V    ───────────── 5V / VCC LED
Mega GND   ───────────── GND LED
Mega pin 2 ──[ 330Ω ]─── DIN LED   (bukan DOUT, ikuti arah panah)
Mega pin 9  ──────────── Buzzer kiri  (+), (−) ke GND
Mega pin 10 ──────────── Buzzer kanan (+), (−) ke GND
```

> ⚠️ **Jangan pakai resistor 1 kΩ di jalur data.** Nilainya terlalu besar untuk sinyal
> WS2812B (800 kHz) dan bisa membuat LED tidak menyala sama sekali. Pakai 220–470 Ω.

Catatan:
- Pin data LED cukup pin **digital** biasa, tidak harus PWM.
- Buzzer hanya memakai `digitalWrite`, jadi juga tidak butuh pin PWM.
- TimerOne memakai Timer1, sehingga PWM di pin 11 dan 12 Mega terganggu. Hindari pin itu.
- 16 LED putih penuh bisa menarik arus sekitar 1 A. Dengan `BRIGHTNESS 80`, daya USB masih cukup.

## Library

Install lewat Arduino IDE (**Sketch → Include Library → Manage Libraries**):

- **Adafruit NeoPixel**
- **TimerOne**

## Cara pakai

1. Buka `BSD_Simulasi/BSD_Simulasi.ino` di Arduino IDE.
2. Pilih **Tools → Board → Arduino Mega or Mega 2560** dan port yang benar.
3. Upload. Simulasi langsung berjalan (`AUTO_DEMO true`).
4. Untuk mengontrol, buka **Serial Monitor** dengan **115200 baud** dan line ending **Newline**.

### Perintah Serial

| Ketik | Fungsi |
|---|---|
| `demo` | Simulasi acak (kiri, kanan, atau keduanya) |
| `kiri` | Simulasi objek hanya dari kiri |
| `kanan` | Simulasi objek hanya dari kanan |
| `dua` | Simulasi objek dari kedua sisi sekaligus |
| `stop` | Hentikan simulasi, semua LED mati |
| `12,0,0,0` | Data manual: `jarakKiri,0,jarakKanan,0` (simulasi berhenti) |

Format data manual wajib berisi **4 nilai dengan 3 koma**. Program hanya memakai nilai
ke-1 (kiri) dan ke-3 (kanan). Jarak `0` berarti tidak ada objek.

## Sistem perhitungan

### Jarak → jumlah LED

```
jumlah LED = ceil( (24 − jarak) × 8 / 24 ) = ceil( (24 − jarak) / 3 )
```

Setiap 3 satuan jarak = 1 LED. Satuan jarak mengikuti data yang dikirim.

| Jarak | LED menyala |
|---|---|
| 0 atau > 24 | 0 (mati) |
| 21 – 24 | 1 |
| 18 – <21 | 2 |
| 15 – <18 | 3 |
| 12 – <15 | 4 |
| 9 – <12 | 5 |
| 6 – <9 | 6 |
| 3 – <6 | 7 |
| > 0 – <3 | 8 (penuh) |

### Warna LED

Posisi LED `i` diubah ke `t = i / 7`:

| t | Warna |
|---|---|
| 0 – 0,3 | Hijau → kuning |
| 0,3 – 0,9 | Kuning → merah |
| ≥ 0,9 | Merah murni |

### Buzzer

Dicek tiap 0,1 detik oleh Timer1:

| Jarak | Pola |
|---|---|
| > 0 – <5 | Bunyi terus |
| 5 – <10 | Bip cepat (0,2 s bunyi, 0,3 s diam) |
| 10 – 15 | Bip lambat (0,2 s bunyi, 0,8 s diam) |
| 0 atau > 15 | Diam |

### Simulasi

Setiap ronde:
1. Pilih acak sisi (kiri / kanan / keduanya), kecepatan 2–8 satuan/detik, dan jarak terdekat 0,5–5,9.
2. **Mendekat** dari jarak 28 sampai jarak terdekat.
3. **Bertahan** 1–3 detik.
4. **Menjauh** sampai keluar jangkauan.
5. **Jeda** 1–2,5 detik, lalu ronde baru.

## Pengaturan

Di bagian atas `BSD_Simulasi.ino`:

| Konstanta | Default | Fungsi |
|---|---|---|
| `LED_PIN` | `2` | Pin data LED |
| `NUM_LEDS` | `16` | Total LED |
| `LEDS_PER_SIDE` | `8` | LED per sisi |
| `BRIGHTNESS` | `80` | Kecerahan (0–255) |
| `MAX_DIST` | `24.0` | Jarak terjauh yang masih menyalakan LED |
| `AUTO_DEMO` | `true` | Simulasi langsung jalan saat Arduino menyala |

## Troubleshooting

| Gejala | Penyebab umum |
|---|---|
| LED tidak menyala sama sekali | Resistor data terlalu besar (mis. 1 kΩ), kabel ke DOUT bukan DIN, GND tidak tersambung, pin salah |
| Warna merah/hijau tertukar | Ganti `NEO_GRB` menjadi `NEO_RGB` |
| Ketik data tapi tidak bereaksi | Line ending Serial Monitor bukan **Newline**, atau format kurang koma |
| LED berkedip / warna acak | Tidak ada resistor data, kabel data terlalu panjang |
