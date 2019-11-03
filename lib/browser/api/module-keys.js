'use strict'

// TODO: Figure out a way to not duplicate this information between here and module-list
// It is currently duplicated as module-list "require"s all the browser API file and the
// remote module in the renderer process depends on that file.  As a result webpack
// includes all the browser API files in the renderer process as well and we want to avoid that

const features = process.electronBinding('features')

// Browser side modules, please sort alphabetically.
module.exports = [
  { name: 'app' },
  { name: 'autoUpdater' },
  { name: 'BrowserView' },
  { name: 'BrowserWindow' },
  { name: 'contentTracing' },
  { name: 'crashReporter' },
  { name: 'dialog' },
  { name: 'globalShortcut' },
  { name: 'ipcMain' },
  { name: 'inAppPurchase' },
  { name: 'Menu' },
  { name: 'MenuItem' },
  { name: 'nativeTheme' },
  { name: 'net' },
  { name: 'netLog' },
  { name: 'Notification' },
  { name: 'powerMonitor' },
  { name: 'powerSaveBlocker' },
  { name: 'protocol' },
  { name: 'screen' },
  { name: 'session' },
  { name: 'systemPreferences' },
  { name: 'TopLevelWindow' },
  { name: 'TouchBar' },
  { name: 'Tray' },
  { name: 'View' },
  { name: 'webContents' },
  { name: 'WebContentsView' }
]

if (features.isViewApiEnabled()) {
  module.exports.push(
    { name: 'BoxLayout' },
    { name: 'Button' },
    { name: 'LabelButton' },
    { name: 'LayoutManager' },
    { name: 'MdTextButton' },
    { name: 'ResizeArea' },
    { name: 'TextField' }
  )
}
