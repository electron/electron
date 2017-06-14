# Panduan Windows Store

Dengan Windows 10, eksekusi win32 yg lama mendapatkan saudara yang baru: *The
Universal Windows Platform*. Format `.appx` yang baru tidak hanya memungkinkan
sejumlah API yang baru dan hebat seperti *Cortana* atau *Push Notifications*,
tetapi juga melalui *Windows Store*, ini akan menyederhanakan instalasi dan update.

Microsoft [telah mengembangkan sebuah alat yang mengkompilasi aplikasi Elektron sebagai paket `.appx`][electron-windows-store], memungkinkan *developer* untuk menggunakan beberapa
sarana yang dapat ditemukan di model aplikasi baru. Panduan ini menjelaskan cara
menggunakannya - dan kemampuan dan keterbatasan paket Electron AppX.

## Latar Belakang dan Persyaratan

Windows 10 "Anniversary Update" dapat menjalankan win32 `.exe` *binaries* dengan cara
meluncurkan mereka bersama-sama dengan *filesystem* virtual dan pendaftaran . Keduanya
dibuat saat kompilasi dengan menjalankan aplikasi and instalasi di dalam *Windows
Container*, memungkinkan *Windows* untuk mengidentifikasi secara tepat modifikasi
sistem operasi mana yang dilakukan saat instalasi. Memasangkan eksekusi
*filesystem* virtual dan pendaftaran virtual yang memungkinkan *Windows* untuk
menjalankan *one-click* instalasi and menghapus instalasi.

Selain itu, exe diluncurkan di dalam bentuk appx - yang berarti bisa menggunakan
API banyak yang tersedia di *Windows Universal Platform*. Untuk mendapatkan
kemampuan yang lebih, aplikasi Electron dapat dipasangkan dengan *UWP background task*
tersembunyi yang dapat diluncurkan bersamaan dengan `exe` - seperti diluncurkan
sebagai dampingan untuk menjalankan *tasks* yang berjalan di *background*,
menerima *push-notification*, atau untuk berkomunikasi dengan aplikasi UWP lainnya.

Untuk mengkompilasi aplikasi Elektron yang ada, pastikan anda memenuhi
persyaratan berikut:

* Windows 10 with Anniversary Update (dikeluarkan August 2nd, 2016)
* The Windows 10 SDK, [unduh disini][windows-sdk]
* Setidaknya Node 4 (untuk mengecek, jalankan `node -v`)


Kemudian, instal `electron-windows-store` CLI:

```
npm install -g electron-windows-store
```

## Step 1:  Kemas Aplikasi Elektron Anda

Kemas aplikasi menggunakan [electron-packager][electron-packager] (atau alat serupa).
Pastikan untuk menghapus `node_modules` yang tidak anda perlukan dalam aplikasi akhir
anda, karena modul yang tidak anda butuhkan hanya akan meningkatkan ukuran aplikasi anda.

Outputnya kira-kira akan terlihat seperti ini:
```
├── Ghost.exe
├── LICENSE
├── content_resources_200_percent.pak
├── content_shell.pak
├── d3dcompiler_47.dll
├── ffmpeg.dll
├── icudtl.dat
├── libEGL.dll
├── libGLESv2.dll
├── locales
│   ├── am.pak
│   ├── ar.pak
│   ├── [...]
├── natives_blob.bin
├── node.dll
├── resources
│   ├── app
│   └── atom.asar
├── snapshot_blob.bin
├── squirrel.exe
└── ui_resources_200_percent.pak
```


## 2: Menjalankan *electron-windows-store*

Dari *PowerShell* (jalankan sebagai "Administrator"), jalankan
`Electron-windows-store` dengan parameter yang dibutuhkan, menggunakan kedua
direktori *input* dan *output*, nama dan versi aplikasi, dan konfirmasi
`Node_modules` harus di *flatten*.

```
electron-windows-store `
    --input-directory C:\myelectronapp `
    --output-directory C:\output\myelectronapp `
    --flatten true `
    --package-version 1.0.0.0 `
    --package-name myelectronapp
```

Setelah dijalankan, alat ini akan mulai bekerja: Ia akan menerima aplikasi Elektron
anda sebagai *input*, *flattening* `node_modules`. Kemudian, ia akan mengarsipkan
aplikasi anda sebagai `app.zip`. Dengan menggunakan *installer* dan *Windows Container*
, alat ini menciptakan paket AppX yang "diperluas" - termasuk *Windows Application
Manifest* (`AppXManifest.xml`) berserta dengan sistem *virtual file* dan pendaftaran
virtual di dalam map *output* anda.


Setelah *file* AppX yang diperluas telah dibuat, alat ini menggunakan
*Windows App Packager* (`MakeAppx.exe`) untuk menggabungkan paket AppX menjadi satu
*file* dari file-file yang ada di *disk*. Akhirnya, alat ini juga bisa digunakan
untuk membuat sertifikat terpercaya di komputer anda untuk menandatangani paket
AppX yang baru. Dengan paket AppX yang telah ditandatangani, CLI juga bisa
secara otomatis menginstal paket di mesin anda.


## 3: Menggunakan Paket AppX

Untuk menjalankan paket, pengguna akan memerlukan Windows 10 dengan apa
yang disebutnya *"Anniversary Update"* - rincian tentang cara memperbarui Windows
dapat ditemukan [di sini][how-to-update].

Di sisi lain dari aplikasi-aplikasi UWP tradisional, aplikasi yang terpaket saat ini
perlu menjalani proses verifikasi manual, yang dapat anda terapkan
[disini][centennial-campaigns]. Sementara itu, semua pengguna bisa menginstal
paket anda dengan mengklik dua kali, oleh sebab itu, pengiriman submisi ke toko
tidak diperlukan jika anda hanya mencari metode instalasi yang mudah. Di lingkungan
yang dikelola (biasanya perusahaan), `Add-AppxPackage` [PowerShell Cmdlet dapat digunakan untuk menginstalnya secara otomatis][add-appxpackage].

Keterbatasan penting lainnya adalah paket AppX yang telah dikompilasi masih berisi
*Win32 executable* - dan karena itu tidak akan berjalan di *Xbox*, *HoloLens*,
atau Telepon.


## Opsional: Tambahkan Fitur UWP menggunakan *BackgroundTask*

Anda dapat memasangkan aplikasi Elektron Anda dengan tugas *background* UWP yang
tersembunyi yang akan memanfaatkan sepenuhnya fitur Windows 10 - seperti *push-notification*,
integrasi Cortana, atau *live tiles*.

Untuk mencari tahu bagaimana aplikasi Elektron yang menggunakan *background task*
untuk mengirim *toast notification* dan *live tiles*, [lihat contoh yang disediakan Microsoft][background-task].


## Opsional: Mengkonversi menggunakan *Container Virtualization*

Untuk menghasilkan paket AppX, `elektron-windows-store` CLI menggunakan *template*
yang seharusnya bekerja untuk sebagian besar aplikasi Electron. Namun, jika anda
menggunakan *custom installer*, atau jika anda mengalami masalah dengan paket
yang dihasilkan, anda dapat mencoba membuat paket menggunakan kompilasi dengan
bantuan Windows Container - di dalam mode itu, CLI akan menginstal dan menjalankan
aplikasi Anda di dalam *Windows Container* yang kosong untuk menentukan
modifikasi apa yang aplikasi Anda lakukan pada sistem operasi.

Sebelum menjalankan CLI, anda harus mengatur *"Windows Desktop App Converter"*.
Ini akan memakan waktu beberapa menit, tapi jangan khawatir - anda hanya perlu
melakukan ini sekali saja. Unduh *Desktop App Converter* dari [di sini][app-converter].
Anda akan menerima dua file: `DesktopAppConverter.zip` dan` BaseImage-14316.wim`.

1. *Unzip* `DesktopAppConverter.zip`. Dari PowerShell (dibuka dengan
  "jalankan sebagai Administrator", pastikan bahwa kebijakan eksekusi sistem
anda mengizinkan untuk menjalankan semua yang ingin dijalankan dengan menggunakan `Set-ExecutionPolicy bypass`.
2. Kemudian, jalankan instalasi *Desktop App Converter*, dengan menggunakan lokasi
*Windows Base Image* (di unduh sebagai `BaseImage-14316.wim`), dengan cara memanggil
perintah `. \ DesktopAppConverter.ps1 -Setup -BaseImage. \ BaseImage-14316.wim`.
3. Jika menjalankan perintah tersebut menyebabkan *reboot*, silakan *restart*
mesin anda dan mejalankan perintah yang telah disebutkan diatas setelah berhasil
*restart*.

Setelah instalasi telah berhasil, anda dapat melajutkan untuk mengkompilasi
aplikasi Electron anda.

[windows-sdk]: https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk
[app-converter]: https://www.microsoft.com/en-us/download/details.aspx?id=51691
[add-appxpackage]: https://technet.microsoft.com/en-us/library/hh856048.aspx
[electron-packager]: https://github.com/electron-userland/electron-packager
[electron-windows-store]: https://github.com/catalystcode/electron-windows-store
[background-task]: https://github.com/felixrieseberg/electron-uwp-background
[centennial-campaigns]: https://developer.microsoft.com/en-us/windows/projects/campaigns/desktop-bridge
[how-to-update]: https://blogs.windows.com/windowsexperience/2016/08/02/how-to-get-the-windows-10-anniversary-update
