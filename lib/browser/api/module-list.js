'use strict'

const features = process.atomBinding('features')

// Browser side modules, please sort alphabetically.
module.exports = [
  { name: 'app', file: 'app' },
  { name: 'autoUpdater', file: 'auto-updater' },
  { name: 'BrowserView', file: 'browser-view' },
  { name: 'BrowserWindow', file: 'browser-window' },
  { name: 'contentTracing', file: 'content-tracing' },
  { name: 'crashReporter', file: 'crash-reporter' },
  { name: 'dialog', file: 'dialog' },
  { name: 'globalShortcut', file: 'global-shortcut' },
  { name: 'ipcMain', file: 'ipc-main' },
  { name: 'inAppPurchase', file: 'in-app-purchase' },
  { name: 'Menu', file: 'menu' },
  { name: 'MenuItem', file: 'menu-item' },
  { name: 'net', file: 'net' },
  { name: 'netLog', file: 'net-log' },
  { name: 'Notification', file: 'notification' },
  { name: 'powerMonitor', file: 'power-monitor' },
  { name: 'powerSaveBlocker', file: 'power-save-blocker' },
  { name: 'protocol', file: 'protocol' },
  { name: 'screen', file: 'screen' },
  { name: 'session', file: 'session' },
  { name: 'systemPreferences', file: 'system-preferences' },
  { name: 'TopLevelWindow', file: 'top-level-window' },
  { name: 'TouchBar', file: 'touch-bar' },
  { name: 'Tray', file: 'tray' },
  { name: 'View', file: 'view' },
  { name: 'webContents', file: 'web-contents' },
  { name: 'WebContentsView', file: 'web-contents-view' }
]

if (features.isViewApiEnabled()) {
  module.exports.push(
    { name: 'BoxLayout', file: 'views/box-layout' },
    { name: 'Button', file: 'views/button' },
    { name: 'LabelButton', file: 'views/label-button' },
    { name: 'LayoutManager', file: 'views/layout-manager' },
    { name: 'MdTextButton', file: 'views/md-text-button' },
    { name: 'ResizeArea', file: 'views/resize-area' },
    { name: 'TextField', file: 'views/text-field' }
  )
}
