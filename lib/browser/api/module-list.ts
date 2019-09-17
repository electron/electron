// TODO: Updating this file also required updating the module-keys file

const features = process.electronBinding('features')

// Browser side modules, please sort alphabetically.
export const browserModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'app', loader: () => require('./app') },
  { name: 'autoUpdater', loader: () => require('./auto-updater') },
  { name: 'BrowserView', loader: () => require('./browser-view') },
  { name: 'BrowserWindow', loader: () => require('./browser-window') },
  { name: 'contentTracing', loader: () => require('./content-tracing') },
  { name: 'crashReporter', loader: () => require('./crash-reporter') },
  { name: 'dialog', loader: () => require('./dialog') },
  { name: 'globalShortcut', loader: () => require('./global-shortcut') },
  { name: 'ipcMain', loader: () => require('./ipc-main') },
  { name: 'inAppPurchase', loader: () => require('./in-app-purchase') },
  { name: 'Menu', loader: () => require('./menu') },
  { name: 'MenuItem', loader: () => require('./menu-item') },
  { name: 'nativeTheme', loader: () => require('./native-theme') },
  { name: 'net', loader: () => require('./net') },
  { name: 'netLog', loader: () => require('./net-log') },
  { name: 'Notification', loader: () => require('./notification') },
  { name: 'powerMonitor', loader: () => require('./power-monitor') },
  { name: 'powerSaveBlocker', loader: () => require('./power-save-blocker') },
  { name: 'protocol', loader: () => require('./protocol') },
  { name: 'screen', loader: () => require('./screen') },
  { name: 'session', loader: () => require('./session') },
  { name: 'systemPreferences', loader: () => require('./system-preferences') },
  { name: 'TopLevelWindow', loader: () => require('./top-level-window') },
  { name: 'TouchBar', loader: () => require('./touch-bar') },
  { name: 'Tray', loader: () => require('./tray') },
  { name: 'View', loader: () => require('./view') },
  { name: 'webContents', loader: () => require('./web-contents') },
  { name: 'WebContentsView', loader: () => require('./web-contents-view') }
]

if (features.isViewApiEnabled()) {
  browserModuleList.push(
    { name: 'BoxLayout', loader: () => require('./views/box-layout') },
    { name: 'Button', loader: () => require('./views/button') },
    { name: 'LabelButton', loader: () => require('./views/label-button') },
    { name: 'LayoutManager', loader: () => require('./views/layout-manager') },
    { name: 'MdTextButton', loader: () => require('./views/md-text-button') },
    { name: 'ResizeArea', loader: () => require('./views/resize-area') },
    { name: 'TextField', loader: () => require('./views/text-field') }
  )
}
