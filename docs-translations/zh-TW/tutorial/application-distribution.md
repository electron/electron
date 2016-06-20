# 應用程式部署

要部屬你的 Electron 應用程式，你需要把你的應用程式資料夾命名為 `app`，並放置於 Electron 的資源目錄下 (在 macOS 中位在 `Electron.app/Contents/Resources/`  而 Linux 和 Windows 的是在 `resources/`)，例如：

macOS:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

Windows 和 Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

然後執行 `Electron.app` (或者在 Linux 中是 `electron`， Windows 中是 `electron.exe`)，然後 Electron 將會啟動你的應用程式，目錄 `electron` 會接著被部署給最後使用者。

## 打包你的應用程式成一個檔案

除了透過複製所有原始檔案來發布你的應用程式，你也可以使用 [asar](https://github.com/atom/asar) 來打包你的應用程式為一個壓縮檔，來避免暴露你的原始碼給使用者。

要使用 `asar` 壓縮檔來取代 `app` 資料夾，你需要重新命名該壓縮檔為 `app.asar`，然後如下所示把它放到 Electron 的資源目錄中，接著 Electron 就會試著讀取壓縮檔並從它開始執行。

macOS:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

Windows 和 Linux:

```text
electron/resources/
└── app.asar
```

更多詳細的介紹請參閱 [應用程式打包](application-packaging.md).

## 重新塑造你的下載執行檔品牌形象

當你完成 Electron 的應用程式打包後，在發布給使用者前，你會想想要重新塑造你的 Electron。

### Windows

你可以重新命名 `electron.exe` 為任何你喜歡的名稱，然後透過像是 [rcedit](https://github.com/atom/rcedit)
的工具來編輯它的圖示(icon)和其他資訊。

### macOS

你可以重新命名 `Electron.app` 為任何你喜歡的名稱，另外你也需要重新命名下列檔案中的 `CFBundleDisplayName`、`CFBundleIdentifier` 和 `CFBundleName` 欄位：

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

你也可以重新命名 helper 應用程式來避免在活動監視器中秀出 `Electron Helper`
，但請確認你有重新命名 helper 應用程式的可執行檔名稱。

重新命名後的應用程式檔案結構可能長得相這樣：

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

你可以重新命名 `electron` 可執行檔為任何你想要的名字。

## 透過重新建置 Electron 原始碼重塑品牌形象

你也可以透過改變產品名稱和重新建置原始碼來重塑 Electron 的品牌形象，要這麼做的話，你需要修改 `atom.gyp` 檔案並在重新建置前清理已建置的所有檔案。

### grunt-build-atom-shell

手動取得 Electron 的原始碼和重新建置可能事件複雜的事，因此有一個建立好的 Grunt 任務(task)可以自動化的處理這些事情：
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

這個任務將會自動的處理編輯 `.gyp` 檔，建置原始碼，然後重新建置你的應用程式原生 Node 模組來符合新的可執行檔名稱。

## 打包工具

除了手動打包你的應用程式，你也可以選擇使用第三方打包工具來幫助你：

* [electron-packager](https://github.com/maxogden/electron-packager)
* [electron-builder](https://github.com/loopline-systems/electron-builder)
