Будь ласка, переконайтесь що ви використовуєте документацію, яка відповідає вашій версії Electron.
Номер версії повинен бути присутнім в URL адресі сторінки. Якщо це не так, Ви можливо, 
використовуєте документацію із development гілки, 
яка може містити зміни в API, які не сумісні з вашою версією Electron.
Якщо це так, тоді Ви можете переключитись на іншу версію документації 
із списку [доступних версій](http://electron.atom.io/docs/) на atom.io,
або якщо ви використовуєте інтеррфейс GitHub, 
тоді відкрийте список "Switch branches/tags" і виберіть потрібну вам 
версію із списку тегів.  

## Довідник

* [Підтримувані платформи](tutorial/supported-platforms.md)
* [Application Distribution](tutorial/application-distribution.md)
* [Mac App Store Submission Guide](tutorial/mac-app-store-submission-guide.md)
* [Упаковка додатку](tutorial/application-packaging.md)
* [Використання Native Node модулів](tutorial/using-native-node-modules.md)
* [Debugging Main Process](tutorial/debugging-main-process.md)
* [Використання Selenium and WebDriver](tutorial/using-selenium-and-webdriver.md)
* [DevTools Extension](tutorial/devtools-extension.md)
* [Використання Pepper Flash плагіну](tutorial/using-pepper-flash-plugin.md)

## Підручники

* [Швидкий старт](tutorial/quick-start.md)
* [Desktop Environment Integration](tutorial/desktop-environment-integration.md)
* [Online/Offline Event Detection](tutorial/online-offline-events.md)

## API References

* [Synopsis](api/synopsis.md)
* [Process Object](api/process.md)
* [Supported Chrome Command Line Switches](api/chrome-command-line-switches.md)
* [Environment Variables](api/environment-variables.md)

### Користувальницькі елементи DOM

* [`File` Object](api/file-object.md)
* [`<webview>` Tag](api/web-view-tag.md)
* [`window.open` Function](api/window-open.md)

### Модулі для Main Process:

* [app](api/app.md)
* [autoUpdater](api/auto-updater.md)
* [BrowserWindow](api/browser-window.md)
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
* [webContents](api/web-contents.md)
* [Tray](api/tray.md)

### Модулі для Renderer Process (Web Page):

* [ipcRenderer](api/ipc-renderer.md)
* [remote](api/remote.md)
* [webFrame](api/web-frame.md)

### Модулі для Both Processes:

* [clipboard](api/clipboard.md)
* [crashReporter](api/crash-reporter.md)
* [nativeImage](api/native-image.md)
* [screen](api/screen.md)
* [shell](api/shell.md)

## Розробка

* [Стиль кодування](development/coding-style.md)
* [Source Code Directory Structure](development/source-code-directory-structure.md)
* [Technical Differences to NW.js (formerly node-webkit)](development/atom-shell-vs-node-webkit.md)
* [Build System Overview](development/build-system-overview.md)
* [Build Instructions (macOS)](development/build-instructions-osx.md)
* [Build Instructions (Windows)](development/build-instructions-windows.md)
* [Build Instructions (Linux)](development/build-instructions-linux.md)
* [Setting Up Symbol Server in debugger](development/setting-up-symbol-server.md)
