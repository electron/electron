const features = process.atomBinding('features')

// Browser side modules, please sort alphabetically.
module.exports = [
  {name: 'app', file: 'app'},
  {name: 'autoUpdater', file: 'auto-updater'},
  {name: 'BrowserView', file: 'browser-view'},
  {name: 'BrowserWindow', file: 'browser-window'},
  {name: 'contentTracing', file: 'content-tracing'},
  {name: 'dialog', file: 'dialog'},
  {name: 'globalShortcut', file: 'global-shortcut'},
  {name: 'ipcMain', file: 'ipc-main'},
  {name: 'inAppPurchase', file: 'in-app-purchase'},
  {name: 'Menu', file: 'menu'},
  {name: 'MenuItem', file: 'menu-item'},
  {name: 'net', file: 'net'},
  {name: 'netLog', file: 'net-log'},
  {name: 'Notification', file: 'notification'},
  {name: 'powerMonitor', file: 'power-monitor'},
  {name: 'powerSaveBlocker', file: 'power-save-blocker'},
  {name: 'protocol', file: 'protocol'},
  {name: 'screen', file: 'screen'},
  {name: 'session', file: 'session'},
  {name: 'systemPreferences', file: 'system-preferences'},
  {name: 'TopLevelWindow', file: 'top-level-window'},
  {name: 'TouchBar', file: 'touch-bar'},
  {name: 'Tray', file: 'tray'},
  {name: 'View', file: 'view'},
  {name: 'webContents', file: 'web-contents'},
  {name: 'WebContentsView', file: 'web-contents-view'},
  // The internal modules, invisible unless you know their names.
  {name: 'NavigationController', file: 'navigation-controller', private: true}
]

if (features.isViewApiEnabled()) {
  module.exports.push(
    {name: 'BoxLayout', file: 'box-layout'},
    {name: 'Button', file: 'button'},
    {name: 'LabelButton', file: 'label-button'},
    {name: 'LayoutManager', file: 'layout-manager'},
    {name: 'TextField', file: 'text-field'}
  )
}
