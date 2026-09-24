// TODO: Updating this file also required updating the module-keys file

// Browser side modules, please sort alphabetically.
export const browserModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'app', loader: () => require('./app') },
  { name: 'autoUpdater', loader: () => require('./auto-updater') },
  { name: 'BaseWindow', loader: () => require('./base-window') },
  { name: 'BrowserView', loader: () => require('./browser-view') },
  { name: 'BrowserWindow', loader: () => require('./browser-window') },
  { name: 'clipboard', loader: () => require('./clipboard') },
  { name: 'ClipboardItem', loader: () => require('./clipboard-item') },
  { name: 'contentTracing', loader: () => require('./content-tracing') },
  { name: 'crashReporter', loader: () => process._linkedBinding('electron_browser_crash_reporter') },
  { name: 'desktopCapturer', loader: () => process._linkedBinding('electron_browser_desktop_capturer') },
  { name: 'dialog', loader: () => process._linkedBinding('electron_browser_dialog') },
  { name: 'globalShortcut', loader: () => require('./global-shortcut') },
  { name: 'ipcMain', loader: () => require('./ipc-main') },
  { name: 'ImageView', loader: () => process._linkedBinding('electron_browser_image_view').ImageView },
  { name: 'inAppPurchase', loader: () => process._linkedBinding('electron_browser_in_app_purchase').inAppPurchase },
  { name: 'Menu', loader: () => require('./menu') },
  { name: 'MenuItem', loader: () => require('./menu-item') },
  {
    name: 'MessageChannelMain',
    loader: () => process._linkedBinding('electron_browser_message_port').MessageChannelMain
  },
  { name: 'nativeTheme', loader: () => require('./native-theme') },
  { name: 'net', loader: () => require('./net') },
  { name: 'netLog', loader: () => require('./net-log') },
  { name: 'Notification', loader: () => process._linkedBinding('electron_browser_notification').Notification },
  { name: 'powerMonitor', loader: () => require('./power-monitor') },
  { name: 'powerSaveBlocker', loader: () => require('./power-save-blocker') },
  { name: 'pushNotifications', loader: () => require('./push-notifications') },
  { name: 'protocol', loader: () => require('./protocol') },
  { name: 'safeStorage', loader: () => require('./safe-storage') },
  { name: 'screen', loader: () => process._linkedBinding('electron_browser_screen').screen },
  { name: 'ServiceWorkerMain', loader: () => require('./service-worker-main') },
  { name: 'session', loader: () => require('./session') },
  { name: 'sharedTexture', loader: () => require('./shared-texture') },
  { name: 'ShareMenu', loader: () => require('./share-menu') },
  { name: 'systemPreferences', loader: () => require('./system-preferences') },
  { name: 'TouchBar', loader: () => require('./touch-bar') },
  { name: 'Tray', loader: () => require('./tray') },
  { name: 'utilityProcess', loader: () => require('./utility-process') },
  { name: 'View', loader: () => process._linkedBinding('electron_browser_view').View },
  { name: 'webContents', loader: () => require('./web-contents') },
  {
    name: 'WebContentsView',
    loader: () => process._linkedBinding('electron_browser_web_contents_view').WebContentsView
  },
  { name: 'webFrameMain', loader: () => require('./web-frame-main') }
];
