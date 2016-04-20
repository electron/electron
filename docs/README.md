Please make sure that you use the documents that match your Electron version.
The version number should be a part of the page URL. If it's not, you are
probably using the documentation of a development branch which may contain API
changes that are not compatible with your Electron version. If that's the case,
you can switch to a different version of the documentation at the
[available versions](http://electron.atom.io/docs/) list on atom.io, or if
you're using the GitHub interface, open the "Switch branches/tags" dropdown and
select the tag that matches your version.

## FAQ

There are questions that are asked quite often, check this out before creating
an issue:

* [Electron FAQ](faq/electron-faq.md)

## Guides

* [Supported Platforms](tutorial/supported-platforms.md)
* [Application Distribution](tutorial/application-distribution.md)
* [Mac App Store Submission Guide](tutorial/mac-app-store-submission-guide.md)
* [Application Packaging](tutorial/application-packaging.md)
* [Using Native Node Modules](tutorial/using-native-node-modules.md)
* [Debugging Main Process](tutorial/debugging-main-process.md)
* [Using Selenium and WebDriver](tutorial/using-selenium-and-webdriver.md)
* [DevTools Extension](tutorial/devtools-extension.md)
* [Using Pepper Flash Plugin](tutorial/using-pepper-flash-plugin.md)
* [Using Widevine CDM Plugin](tutorial/using-widevine-cdm-plugin.md)
* [Testing on Headless CI Systems (Travis, Jenkins)](tutorial/testing-on-headless-ci.md)

## Tutorials

* [Quick Start](tutorial/quick-start.md)
* [Desktop Environment Integration](tutorial/desktop-environment-integration.md)
* [Online/Offline Event Detection](tutorial/online-offline-events.md)

## API References

* [Synopsis](api/synopsis.md)
* [Process Object](api/process.md)
* [Supported Chrome Command Line Switches](api/chrome-command-line-switches.md)
* [Environment Variables](api/environment-variables.md)

### Custom DOM Elements:

* [`File` Object](api/file-object.md)
* [`<webview>` Tag](api/web-view-tag.md)
* [`window.open` Function](api/window-open.md)

### Modules for the Main Process:

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

### Modules for the Renderer Process (Web Page):

* [desktopCapturer](api/desktop-capturer.md)
* [ipcRenderer](api/ipc-renderer.md)
* [remote](api/remote.md)
* [webFrame](api/web-frame.md)

### Modules for Both Processes:

* [clipboard](api/clipboard.md)
* [crashReporter](api/crash-reporter.md)
* [nativeImage](api/native-image.md)
* [screen](api/screen.md)
* [shell](api/shell.md)

## Development

* [Coding Style](development/coding-style.md)
* [Source Code Directory Structure](development/source-code-directory-structure.md)
* [Technical Differences to NW.js (formerly node-webkit)](development/atom-shell-vs-node-webkit.md)
* [Build System Overview](development/build-system-overview.md)
* [Build Instructions (OS X)](development/build-instructions-osx.md)
* [Build Instructions (Windows)](development/build-instructions-windows.md)
* [Build Instructions (Linux)](development/build-instructions-linux.md)
* [Debug Instructions (Windows)](development/debug-instructions-windows.md)
* [Setting Up Symbol Server in debugger](development/setting-up-symbol-server.md)
