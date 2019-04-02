## Property Updates

The Electron team is currently undergoing an initiative to convert separate getter and setter functions in Electron to bespoke properties with `get` and `set` functionality. During this transition period, both the new properties and old getters and setters of these functions will work correctly and be documented.

## Candidates

* `app` module
  * `badgeCount`
  * `name`
  * `dock`
    * `badge`
* `autoUpdater` module
  * `feedUrl`
* `BrowserWindow`
  * `fullscreen`
  * `simpleFullscreen`
  * `movable`
  * `resizable`
  * `maximizable`
  * `minimizable`
  * `fullscreenable`
  * `closable`
  * `alwaysOnTop`
  * `title`
  * `documentEdited`
  * `hasShadow`
  * `autohideMenuBar`
  * `menubarVisible`
  * `visibleOnAllWorkspaces`
* `crashReporter` module
  * `uploadToServer`
* `DownloadItem` class
  * `savePath`
  * `paused`
* `NativeImage`
  * `templateImage`
* `Session` module
  * `preloads`
* `SystemPreferences` module
  * `appLevelAppearance`
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
