## Property Updates

The Electron team is currently undergoing an initiative to convert separate getter and setter functions in Electron to bespoke properties with `get` and `set` functionality. During this transition period, both the new properties and old getters and setters of these functions will work correctly and be documented.

## Candidates

* `app` module
  * `dock`
    * `badge`
* `autoUpdater` module
  * `feedUrl`
* `BrowserWindow`
  * `fullscreen`
  * `simpleFullscreen`
  * `alwaysOnTop`
  * `title`
  * `documentEdited`
  * `hasShadow`
  * `menubarVisible`
  * `visibleOnAllWorkspaces`
* `crashReporter` module
  * `uploadToServer`
* `Session` module
  * `preloads`
* `webContents` module
  * `zoomFactor`
  * `zoomLevel`
  * `audioMuted`
  * `userAgent`
  * `frameRate`
* `webFrame` modules
  * `zoomFactor`
  * `zoomLevel`
  * `audioMuted`
* `<webview>`
  * `zoomFactor`
  * `zoomLevel`
  * `audioMuted`

## Converted Properties

* `app` module
  * `accessibilitySupport`
  * `applicationMenu`
  * `badgeCount`
  * `name`
* `DownloadItem` class
  * `savePath`
* `BrowserWindow` module
  * `autohideMenuBar`
  * `resizable`
  * `maximizable`
  * `minimizable`
  * `fullscreenable`
  * `movable`
  * `closable`
* `NativeImage`
  * `isMacTemplateImage`
* `SystemPreferences` module
  * `appLevelAppearance`
