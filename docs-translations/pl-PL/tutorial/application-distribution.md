# Dystrybowanie aplikacji

Aby rozpowszechniać aplikacją powinieneś pobrać pre-budowane pliki binarne Electron. Następnie folder zawierający powinien być nazwany `app` i umieszczony w katalogu z zawartością Electron jak w poniższym przykładzie.

na macOS:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

na Windows i Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Then execute `Electron.app` (or `electron` on Linux, `electron.exe` on Windows),
and Electron will start as your app. The `electron` directory will then be
your distribution to deliver to final users.

Następnie uruchom `Electron.app` (lub `electron` na Linux, `electron.exe` na Windows) a Electron uruchomi twoją aplikację. Folder `electron` będzie służył do rozpowszechniania pomiędzy użytkowników.

## Pakowanie twojej aplikacji do pliku
Apart from shipping your app by copying all of its source files, you can also
package your app into an [asar](https://github.com/electron/asar) archive to avoid
exposing your app's source code to users.

To use an `asar` archive to replace the `app` folder, you need to rename the
archive to `app.asar`, and put it under Electron's resources directory like
below, and Electron will then try to read the archive and start from it.

Poza dostarczanie plików źródłowych twojej aplikacji, możesz także spakować je do pojedynczego archiwum [asar](https://github.com/electron/asar) aby uniknąć bezpośredniego ujawniania plików źródłowych twojej aplikacji.

na macOS:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

na Windows i Linux:

```text
electron/resources/
└── app.asar
```

po więcej informacji zajrzyj do [Pakowanie aplikacji](application-packaging.md).

## Przemianowanie z pobranymi plikami binarnymi
Po spakowaniu aplikacji do Electron, możesz chcieć zmienić nieco informacji i sposób w jaki przedstawiają się jej pliki przed opublikowaniem użytkownikom.

### Windows
Możesz zmienić nazwę pliku `electron.exe` na jaką tylko zechcesz i edytować jej ikonę i inne informacje narzędziem jak na przykład [rcedit](https://github.com/atom/rcedit).

### macOS

Możesz zmienić nazwę `Electron.app` na jaką tylko zechcesz, oraz zmienić nazwę pól `CFBundleDisplayName`, `CFBundleIdentifier` oraz `CFBundleName` w podanych plikach:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

Możesz także zmienić nazwę aplikacji pomocniczej, aby uniknąć pojawieniu się `Electron Helper` w monitorze aktywności, ale upewnij się, że zmieniona nazwa to nadal nazwa wykonywanlna.

Struktura przetworzonej aplikacji powinna wyglądać mniej więcej tak:
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

Możesz zmienić nazwę pliku `electron` na jakąkolwiek zechcesz.

## Narzędzia pakujące
Poza pakowaniem ręcznym, możesz wybierać spośród zewnętrznych narzędzi pakujących, które zrobią tę pracę za ciebie.

* [electron-builder](https://github.com/electron-userland/electron-builder)
* [electron-packager](https://github.com/electron-userland/electron-packager)

#### Creating a Custom Release with surf-build

1. Zainstaluj [Surf](https://github.com/surf-build/surf), przez npm:
  `npm install -g surf-build@latest`

2. Stwórz nowy S3 bucket and utwórz w nim strukturę pustych katalogów jak poniżej:

    ```
    - atom-shell/
      - symbols/
      - dist/
    ```

3. Ustaw zmienne środowiskowe:

  * `ELECTRON_GITHUB_TOKEN` - token mogący tworzyć nową wersję na GitHub
  * `ELECTRON_S3_ACCESS_KEY`, `ELECTRON_S3_BUCKET`, `ELECTRON_S3_SECRET_KEY` -
    miejsce gdzie będziesz wysyłał nagłówki node.js jako symbole
  * `ELECTRON_RELEASE` - ustaw na `true` i część wysyłana zostanie uruchomiona, pozostaw nieokreślone a wtedy `surf-build` wykona jedynie sprawdzenie CI-type
  * `CI` - ustaw na true, w innym przypadku nie powiedzie się
  * `GITHUB_TOKEN` - ustaw tak jak `ELECTRON_GITHUB_TOKEN`
  * `SURF_TEMP` - ustaw na `C:\Temp` aby uniknąć problemów ze zbyt długą nazwą ścieżki docelowej
  * `TARGET_ARCH` - ustaw na `ia32` lub `x64`  

4. W pliku `script/upload.py`,  _musisz_ ustawić `ELECTRON_REPO` na twojego forka (`MYORG/electron`),
  zwłaszcza, jeśli jesteś współautorem Electronu.

5. `surf-build -r https://github.com/MYORG/electron -s TWÓJ_COMMIT -n 'surf-PLATFORM-ARCH'`

6. Poczekaj na zakończenie.
