# Application Distribution

Electron uygulamanızı dağıtmak için önce Electron nun [prebuilt mimarilerini]
(https://github.com/electron/electron/releases) indirmeniz gerekmektedir.
Sonrasında, uygulamanızın bulundugu klasör `app` şeklinde isimlendirilmeli ve
Electron kaynaklar klasörüne aşagıda gösterildiği gibi yerleştirilmelidir.
Unutmayın, Electronun prebuilt mimarileri aşağıdaki örneklerde `electron/`
şeklinde belirtilmiştir.


MacOS da:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

Windows ve Linux da:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Ardından `Electron.app` (veya `electron` Linux'da, `electron.exe` Windows'da) şeklinde çalıstırın,
ve Electron uygulama şeklinde çalışacaktır.
`electron` klasörü son kullanıcıya aktaracağınız dağıtımınız olacaktır.

## Uygulamanın bir dosya şeklinde paketlenmesi

Tüm kaynak kodlarını kopyalama yoluyla uygulamanızı dağıtmak haricinde,
uygulamanızı [asar](https://github.com/electron/asar) ile arşiv haline getirerek,
kaynak kodlarınızın kullanıcılar tarafından görülmesini engelliye bilirsiniz.

`app` klasörü yerine `asar` arşiv dosyası kullanmak için, arşiv dosyanızı `app.asar`
şeklinde isimlendirmeniz gerekiyor, ve bu dosyayı Electron'nun kaynak klasörüne aşağıdaki
gibi yerleştirmelisiniz. Böylelikle Electron arşivi okuyup ondan başlayacaktır.


MacOS'da:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

Windows ve Linux'da:

```text
electron/resources/
└── app.asar
```

Daha fazla bilgi için [Application packaging](application-packaging.md).

## İndirilen mimarileri yeniden adlandırma

Uygulamanızı Electron ile paketledikten sonra ve kullanıcılara uygulamanızı dağıtmadan önce
adını değiştirmek isteye bilirsiniz.

### Windows

`electron.exe` istediğiniz şekilde yeniden adlandırabilirsiniz. Icon ve diğer
bilgileri bu gibi araçlar [rcedit](https://github.com/atom/rcedit) ile düzenleye bilirsiniz.

### macOS

`Electron.app`'i istediğiniz şekilde yeniden adlandırabilirsiniz, ve aşağıdaki dosyalarda
`CFBundleDisplayName`, `CFBundleIdentifier` ve `CFBundleName` kısımlarınıda düzenlemelisiniz.

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

Görev yöneticisinde `Electron Helper` şeklinde göstermek yerine,
isterseniz helper uygulamasınında adını değiştire bilirsiniz,
ancak dosyanın adını açılabilir olduğundan emin olun.

Yeniden adlandırılmış uygulamanın klasör yapısı bu şekilde görünecektir:

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

`electron` dosyasını istediğiniz şekilde yeniden adlandırabilirsiniz.

## Paketleme Araçları

Uygulamanızı manuel şekilde paketlemek dışında, üçüncü parti
paketleme araçlarıylada otomatik olarak ayni şekilde paketliye bilirsiniz:

* [electron-builder](https://github.com/electron-userland/electron-builder)
* [electron-packager](https://github.com/electron-userland/electron-packager)

## Kaynaktan yeniden kurulum yoluyla isim değişikliği

Ürün adını değiştirip, kaynaktan kurulum yoluylada Electron'nun adını değiştirmek mümkün.
Bunun için `atom.gyp` dosyasını yeniden modifiye edip, tekrardan temiz bir kurulum yapmalısınız.

### grunt-build-atom-shell

Manuel olarak Electron kodlarını kontrol edip tekrar kurulum yapmak biraz zor olabilir,
bu yüzden tüm bu işlemleri otomatik olarak gerçekleştirecek bir Grunt görevi oluşturuldu:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

Bu görev otomatik olarak `.gyp` dosyasını düzenleyecek, kaynaktan kurulumu gerçekleştirecek,
sonrasında ise uygulamanızın doğal Node modüllerini, yeni yürütülebilen isim ile eşleştirmek icin
tekrardan kuracaktır.

### Özel bir Electron kopyası oluşturma

Electron'un size ait bir kopyasını oluşturmak, neredeyse uygulamanızı kurmak için hiç ihtiyacınız
olmayacak bir işlemdir, "Production Level" uygulamalarda buna dahildir.
`electron-packager` veya `electron-builder` gibi araçlar kullanarak yukarıda ki işlemleri
gerçekleştirmeksizin, "Rebrand" Electron işlemini uygulaya bilirsiniz.

Eğer kendinize ait yüklenemiyen veya resmi versiyondan red edilmiş,
direk olarak Electron a paketlediğiniz C++ kodunuz var ise,
öncelikle Electron'un bir kopyasını oluşturmalısınız.
Electron'nun destekleyicileri olarak, senaryonuzun çalışmasını çok isteriz,
bu yüzden lütfen yapacağınız değişiklikleri Electron'nun resmi versiyonuna
entegre etmeye calışın, bu sizin için daha kolay olacaktır, ve yardimlarınız
için cok minnettar olacağız.

#### surf-build İle Özel Dağıtım oluşturulması

1. Npm yoluyla [Surf](https://github.com/surf-build/surf) yükleyin:
  `npm install -g surf-build@latest`


2. Yeni bir S3 bucket ve aşağıdakı boş klasör yapısını oluşturun:

    ```
    - atom-shell/
      - symbols/
      - dist/
    ```

3. Aşağıdaki Ortam Değişkenlerini ayarlayın:

  * `ELECTRON_GITHUB_TOKEN` - GitHub üzerinden dağıtım oluşturan token
  * `ELECTRON_S3_ACCESS_KEY`, `ELECTRON_S3_BUCKET`, `ELECTRON_S3_SECRET_KEY` -
    node.js bağlantılarını ve sembollerini yükleyeceğiniz yer
  * `ELECTRON_RELEASE` - `true` şeklinde ayarlayın ve yükleme işlemi çalışacaktır,
     yapmamanız halinde, `surf-build` sadece CI-type kontrolü yapacak,
     tüm pull isteklerine uygun hale getirecektir.
  * `CI` - `true` olarak ayarlayın yoksa çalışmayacaktır.
  * `GITHUB_TOKEN` - bununla aynı şekilde ayarlayın `ELECTRON_GITHUB_TOKEN`
  * `SURF_TEMP` -  Windowsda ki 'path too long' sorunundan kaçınmak için `C:\Temp` şeklinde ayarlayın
  * `TARGET_ARCH` -  `ia32` veya `x64` şeklinde ayarlayın

4. `script/upload.py` dosyasında ki `ELECTRON_REPO` kısmını, kendi kopyanız ile değiştirmek _zorundasınız_,
  özellikle eğer bir Electron proper destekleyicisi iseniz.

5. `surf-build -r https://github.com/MYORG/electron -s YOUR_COMMIT -n 'surf-PLATFORM-ARCH'`

6. Kurulum bitene kadar uzunca bekleyin.
