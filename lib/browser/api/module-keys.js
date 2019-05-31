'use strict'

// TODO: Figure out a way to not duplicate this information between here and module-list

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
