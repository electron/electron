使用している Electron のバージョンに応じたドキュメントを使うように確認してください。
ドキュメントのバージョン番号はページの URL の一部になっています。
そうでない場合、おそらくご使用の Electron のバージョンと互換性のない API 変更を含んだ development ブランチのドキュメントを使っているものと思われます。
その場合、atom.io の [available versions](http://electron.atom.io/docs/) リストにある別のバージョンのドキュメントに切り替えることができます。また GitHub で閲覧している場合、"Switch branches/tags" ドロップダウンを開いて、バージョンに対応したタグを選ぶこともできます。

_リンクになっていないリストは未翻訳のものです。_
## FAQ

頻繁に聞かれる質問がありますので、issueを作成する前にこれをチェックしてください。

* [Electron FAQ](faq/electron-faq.md)

## ガイド

* [サポートするプラットフォーム](tutorial/supported-platforms.md)
* [セキュリティ](tutorial/security.md)
* [Electronのバージョン管理](tutorial/electron-versioning.md)
* [アプリケーションの配布](tutorial/application-distribution.md)
* [Mac App Store 提出ガイド](tutorial/mac-app-store-submission-guide.md)
* Windows Store Guide (tutorial/windows-store-guide.md)
* [アプリケーションのパッケージ化](tutorial/application-packaging.md)
* [ネイティブのNodeモジュールを使用する](tutorial/using-native-node-modules.md)
* [メインプロセスのデバッグ](tutorial/debugging-main-process.md)
* [Selenium と WebDriverを使用する](tutorial/using-selenium-and-webdriver.md)
* [DevTools エクステンション](tutorial/devtools-extension.md)
* [Pepper Flashプラグインを使用する](tutorial/using-pepper-flash-plugin.md)
* [Widevine CDMプラグインを使用する](tutorial/using-widevine-cdm-plugin.md)
* [継続的インテグレーションシステムによるテスト(Travis, Jenkins)](tutorial/testing-on-headless-ci.md)


# チュートリアル

* [クイックスタート](tutorial/quick-start.md)
* [デスクトップ環境の統合](tutorial/desktop-environment-integration.md)
* [オンライン/オフライン イベントの検知](tutorial/online-offline-events.md)

## API リファレンス

* [概要](api/synopsis.md)
* [Process Object](api/process.md)
* [サポートしているChromeコマンドラインスイッチ](api/chrome-command-line-switches.md)
* [環境変数](api/environment-variables.md)

### カスタムDOM要素:

* [`File` Object](api/file-object.md)
* [`<webview>` タグ](api/web-view-tag.md)
* [`window.open` 関数](api/window-open.md)

### Main Processのモジュール:

* [app](api/app.md)
* [autoUpdater](api/auto-updater.md)
* BrowserWindow (api/browser-window.md)
    * [フレームの無いウィンドウ](api/frameless-window.md)
* [contentTracing](api/content-tracing.md)
* [dialog](api/dialog.md)
* [globalShortcut](api/global-shortcut.md)
* [ipcMain](api/ipc-main.md)
* [Menu](api/menu.md)
* [MenuItem](api/menu-item.md)
* [powerMonitor](api/power-monitor.md)
* [powerSaveBlocker](api/power-save-blocker.md)
* [protocol](api/protocol.md)
* [session](api/session.md)
* [systemPreferences](api/system-preferences.md)
* [Tray](api/tray.md)
* webContents (api/web-contents.md)

### Renderer Processのモジュール (Web Page):

* [desktopCapturer](api/desktop-capturer.md)
* [ipcRenderer](api/ipc-renderer.md)
* [remote](api/remote.md)
* [webFrame](api/web-frame.md)

###  両方のProcessのモジュール :

* [clipboard](api/clipboard.md)
* [crashReporter](api/crash-reporter.md)
* [nativeImage](api/native-image.md)
* [screen](api/screen.md)
* [shell](api/shell.md)

## 開発


* Coding Style (development/coding-style.md)
* Source Code Directory Structure (development/source-code-directory-structure.md)
* Technical Differences to NW.js  (formerly node-webkit)(development/atom-shell-vs-node-webkit.md)
* Build System Overview (development/build-system-overview.md)
* Build Instructions (macOS) (development/build-instructions-osx.md)
* Build Instructions (Windows) (development/build-instructions-windows.md)
* Build Instructions (Linux) (development/build-instructions-linux.md)
* Debug Instructions (Windows) (development/debug-instructions-windows.md)
* Setting Up Symbol Server in debugger (development/setting-up-symbol-server.md)
