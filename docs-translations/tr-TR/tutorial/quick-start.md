# Hızlı Başlangıç

Electron, zengin native(işletim sistemi) API runtime sağlayarak, saf Javascript
ile masaüstü uygulamalar geliştirmenize yarar. Electron'u Node.js in, web serverları
yerine masaüstü uygulamalara odaklanmış bir variyasyonu olarak kabul edebilirsiniz.

Bu Electronun, grafik kullanıcı arayüzüne bir JavaScript bağlantısı olduğu
anlamına gelmez. Aksine, Electron web sayfalarını GUI'si olarak kullanır,
yani onu Javascript tarafından kontrol edilen bir minimal Chromium tarayıcısı
olarak görebilirsiniz.

### Ana İşlem

Electron da, `package.json` nun `main` skriptini cağıran işlem _the main process__ dir.
Ana işlemde çalışan bu script, GUI'yi web sayfalarını oluşturarak gösterebilir.

### Render İşlemi

Electron, web sayfalarını görüntülemek için Chromium kullandığından,
aynı zamanda Chromiumun multi-işlem mimarisinide kullanmaktadır.
Electron da calıştırılan her web sayfası, __the renderer process__
adı altında kendi işlemlerini çalıştırırlar.

Normal tarayıcılarda, web sayfaları genellikle korumalı bir ortamda çalışır ve
yerel kaynaklara erişmesine izin verilmez. Bununla birlikte, elektron kullanıcıları,
alt düzey işletim sistemi etkileşimlerine izin veren web sayfalarında
Node.js API'lerini kullanma imkanina sahiplerdir.

### Ana işlem ile render işlemi arasındaki farklar

Ana işlem, `BrowserWindow` örneklerini oluşturarak, web sayfalarını hazır
hale getirir. Her bir `BrowserWindow`  örneği web sayfasını kendi render
işleminde çalıştırır. Eger bir `BrowserWindow`  örneği ortadan kaldırıldıysa,
bununla bağlantılı olan render işlemide aynı şekilde sonlandırılır.

Ana işlem tüm web sayfaları ve onların ilgili olduğu render işlemlerini yönetir.
Her bir render işlemi izole edilmiş ve sadece kendisinde çalışan web sayfasıyla ilgilenir.

Native GUI ile çalışan API ları web sayfalarında çalıştırmaya izin verilmemektedir,
çünkü native GUI kaynaklarının web sayfalarında yönetimi çok tehlikeli ve
kaynakların sızdırılması gayet kolaydır. Eğer GUI operasyonlarını bir web sayfasinda
gerçekleştirmek istiyorsanız, web sayfasının render işlemi, ana işlem ile, bu tür
işlemleri gerçekleştirilmesini talep etmek için kommunikasyon halinde olmalı.

Electron da ana işlem ve render işlemi arasında birden fazla kommunikasyon yolu vardır.
[`ipcRenderer`](../api/ipc-renderer.md) gibi ve mesaj gönderimi icin
[`ipcMain`](../api/ipc-main.md) modülleri, RPC tarzında kommunikasyon
için ise [remote](../api/remote.md) modülü barındırmakta.
Ayrıca SSS başlıkları [how to share data between web pages][share-data] adresinde bulunabilir.

## İlk Electron uygulamanızı yazın

Electron uygulaması genellikle aşağıdaki gibi yapılandırılmıştır:

```text
your-app/
├── package.json
├── main.js
└── index.html
```

`package.json` dosyasının formatı tamamen Node modüllerine benzer veya aynıdır ve
`main` şeklinde adlandırılmış script uygulamanızı başlatan komut dosyasıdır,
bu komut dosyası daha sonra main process'i çalıştıracak dosyadır.
`package.json` dosyasınızın bir örneği aşağıdaki gibi olabilir:


```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__Note__: Eğer `package.json` dosyasında `main` kısmı bulunmuyorsa, Electron standart olarak
`index.js` dosyasını cağıracaktır.

`main.js` dosyası pencereleri oluşturur, sistem durumlarını handle eder, tipik bir
örnek asağıdaki gibidir:

```javascript
const {app, BrowserWindow} = require('electron')
const path = require('path')
const url = require('url')

// Pencere objesini daima global referans olarak tanımla, aksi takdirde,
// eğer JavaScript objesi gereksiz veriler toplayacağı için, pencere
// otomatik olarak kapanacaktır.

let win

function createWindow () {
  // Tarayıcı pencerelerini oluşturur.
  win = new BrowserWindow({width: 800, height: 600})

  // ve uygulamanın index.html sayfasını yükler.
  win.loadURL(url.format({
    pathname: path.join(__dirname, 'index.html'),
    protocol: 'file:',
    slashes: true
  }))

  // DevTools her uygulama başlatıldığında açılır.

  win.webContents.openDevTools()

  // Pencere kapandıktan sonra çağrılacaktır.
  win.on('closed', () => {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    win = null
  })
}

// Bu metod Electronun başlatılması tamamlandıktan sonra
// çagrılacak ve yeni tarayıcı pencereleri açmaya hazır hale gelecektir.
// Bazı API lar sadece bu event gerçekleştikten sonra kullanılabilir.

app.on('ready', createWindow)

// Eğer tüm pencereler kapandıysa, çıkış yap.

app.on('window-all-closed', () => {
  // On macOS it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  // On macOS it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (win === null) {
    createWindow()
  }
})

// Bu sayfada, uygulamanızın spesifik main process kodlarını dahil edebilirsiniz.
// Aynı zamanda bu kodları ayrı dosyalar halinde oluştura bilir
// ve buraya require yoluyla ekleye bilirsiniz.
```

Son olarak `index.html` yani göstermek istediğiniz web sayfası:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using node <script>document.write(process.versions.node)</script>,
    Chrome <script>document.write(process.versions.chrome)</script>,
    and Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## Uygulamanızı çalıştırın

`main.js`, `index.html`, ve `package.json` dosyalarını oluşturduktan sonra,
uygulamanızı lokal olarak test ederek, doğru çalışıp çalışmadığını
test etmek isteye bilirsiniz. O halde aşağıdaki yönergeleri takip edin:

### `electron`

[`electron`](https://github.com/electron-userland/electron-prebuilt),
Electron'un pre-compiled versiyonunu içeren bir `npm` modülüdür.


Eğer bunu global olarak `npm` yoluyla yüklediyseniz, o halde sadece aşağıdaki komutu
uygulamanızın kaynak klasöründe çalıstırmanız yeterlidir:

```bash
electron .
```

Eğer lokal olarak yüklediyseniz, o zaman aşağıda ki gibi
çalıştırın:

#### macOS / Linux

```bash
$ ./node_modules/.bin/electron .
```

#### Windows

```bash
$ .\node_modules\.bin\electron .
```

### Manuel olarak indirilmiş Electron mimarisi

Eğer Electronu manuel olarak indirdiyseniz, aynı zamanda dahili olan
mimariyide kullanarak, uygulamanızı çalıştıra bilirsiniz.

#### Windows

```bash
$ .\electron\electron.exe your-app\
```

#### Linux

```bash
$ ./electron/electron your-app/
```

#### macOS

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` Electron un dağı₺tım paketinin bir parçasıdır,
bunu [adresinden](https://github.com/electron/electron/releases) indirebilirsiniz.

### Dağıtım olarak çalıştır

Uygulamanızı yazdıktan sonra, bir dağıtım oluşturmak için  
[Application Distribution](./application-distribution.md)
sayfasında ki yönergeleri izleyin ve daha sonra arşivlenmiş uygulamayı çalıştırın.

### Örneği deneyin

[`electron/electron-quick-start`](https://github.com/electron/electron-quick-start) repository klonlayarak bu eğitimdeki kodu çalıştıra bilirsiniz.

**Note**: Bu işlemleri uygulamak için [Git](https://git-scm.com) ve [Node.js](https://nodejs.org/en/download/) ([npm](https://npmjs.org) da bununla birlikte gelir) sisteminizde yüklü olması gerekmektedir.

```bash
# Repository klonla
$ git clone https://github.com/electron/electron-quick-start
# Electron repositorye git
$ cd electron-quick-start
# Gerekli kütüphaneleri yükle
$ npm install
# Uygulamayı çalıştır
$ npm start
```

Daha fazla örnek uygulama için, harika electron topluluğu tarafından oluşturulan,
[list of boilerplates](https://electron.atom.io/community/#boilerplates)
sayfasını ziyaret edin.

[share-data]: ../faq.md#how-to-share-data-between-web-pages
