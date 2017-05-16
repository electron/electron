# Menggunakan *Plugin Widevine CDM*

Di Electron, anda dapat menggunakan *plugin Widevine CDM* yang disertakan dengan
*browser* Chrome.

## Mendapatkan *plugin*

Elektron tidak disertakan dengan *plugin Widevine CDM* karena alasan lisensi,
untuk mendapatkanny, anda perlu menginstal *browser* Chrome resmi terlebih dahulu,
yang seharusnya cocok dengan arsitektur dan versi Chrome dari Elektron yang anda
gunakan.

**Catatan:** Versi utama *browser* Chrome harus sama dengan versi Chrome yang
digunakan oleh Electron, jika tidak cocok, *plugin* tidak akan bekerja sekalipun
`Navigator.plugins` akan menunjukkan bahwa ia telah dimuat.

### Windows & macOS

Buka `chrome://components/` di *browser* Chrome, cari `WidevineCdm` dan pastikan
apabila itu *up-to-date*, maka Anda dapat menemukan semua plugin binari dari direktori
`APP_DATA / Google / Chrome / WidevineCDM / VERSION / _platform_specific / PLATFORM_ARCH /`.



`APP_DATA` adalah lokasi sistem untuk menyimpan data aplikasi, di sistem Windows
itu ada di `% LOCALAPPDATA%`, di macOS itu ada di `~ / Library / Application Support`.
`VERSION` adalah versi *Widevine CDM plugin*, seperti `1.4.8.866`. `PLATFORM` adalah `mac`
atau `win`. `ARCH` adalah` x86` atau `x64`.

Di Windows, binari yang dibutuhkan adalah `widevinecdm.dll` dan
`Widevinecdmadapter.dll`, di macOS adalah` libwidevinecdm.dylib` dan
`Widevinecdmadapter.plugin`. Anda bisa menyalinnya ke manapun Anda suka, tapi
mereka harus di letakkan bersama.


### Linux


Di Linux, binari plugin disertakan bersama dengan *browser* Chrome, anda bisa
menemukannya di `/ opt / google / chrome`, nama filenya adalah` libwidevinecdm.so` dan
`Libwidevinecdmadapter.so`.



## Menggunakan *plugin*

Setelah mendapatkan *file* plugin, anda harus meneruskan `widevinecdmadapter`
ke Electron dengan baris perintah penghubung `--widevine-cdm-path`, dan versi
pluginnya dengan pengubung `--widevine-cdm-version`.

**Catatan:** Meskipun hanya binari `widevinecdmadapter` yang dilewatkan ke Electron, binari
`Widevinecdm` harus disertakan bersama.

Penghubung baris perintah harus dilewati sebelum `ready` dari` app` modul dipancarkan,
dan halaman yang menggunakan plugin ini harus mempunyai *plugin* yang sudah diaktifkan.


Contoh kode:

```javascript
const {app, BrowserWindow} = require('electron')

// Anda harus melewatkan filename `widevinecdmadapter` di sini, yang disebut adalah:
// * `widevinecdmadapter.plugin` on macOS,
// * `libwidevinecdmadapter.so` on Linux,
// * `widevinecdmadapter.dll` on Windows.
app.commandLine.appendSwitch('widevine-cdm-path', '/path/to/widevinecdmadapter.plugin')
// Versi plugin dapat didapatkan dari halaman `chrome://plugins` di Chrome.
app.commandLine.appendSwitch('widevine-cdm-version', '1.4.8.866')

let win = null
app.on('ready', () => {
  win = new BrowserWindow({
    webPreferences: {
      // `plugins` harus diaktifkan.
      plugins: true
    }
  })
  win.show()
})
```

## Verifikasi plugin

Untuk memverifikasi jika plugin telah berhasil, anda dapat menggunakan cara berikut:

* Buka *devtools* dan periksa apakah `navigator.plugins` menyertakan *Widevine
Plugin CDM*
* Buka https://shaka-player-demo.appspot.com/ dan muat manifes yang menggunakan
`Widevine`.
* Buka http://www.dash-player.com/demo/drm-test-area/, periksa apakah di halamannya
terdapat `bitdash uses Widevine in your browser`, lalu putar videonya.
