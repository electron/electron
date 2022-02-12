// TODO: Updating this file also required updating the module-keys file

// Browser side modules, please sort alphabetically.
export const browserModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'app', loader: () => require('./app') },
  { name: 'autoUpdater', loader: () => require('./auto-updater') },
  { name: 'BaseView', loader: () => require('./base-view') },
  { name: 'BaseWindow', loader: () => require('./base-window') },
  { name: 'BrowserView', loader: () => require('./browser-view') },
  { name: 'BrowserWindow', loader: () => require('./browser-window') },
  { name: 'ContainerView', loader: () => require('./container-view') },
  { name: 'contentTracing', loader: () => require('./content-tracing') },
  { name: 'crashReporter', loader: () => require('./crash-reporter') },
  { name: 'dialog', loader: () => require('./dialog') },
  { name: 'globalShortcut', loader: () => require('./global-shortcut') },
  { name: 'ipcMain', loader: () => require('./ipc-main') },
  { name: 'inAppPurchase', loader: () => require('./in-app-purchase') },
  { name: 'Menu', loader: () => require('./menu') },
  { name: 'MenuItem', loader: () => require('./menu-item') },
  { name: 'MessageChannelMain', loader: () => require('./message-channel') },
  { name: 'nativeTheme', loader: () => require('./native-theme') },
  { name: 'net', loader: () => require('./net') },
  { name: 'netLog', loader: () => require('./net-log') },
  { name: 'Notification', loader: () => require('./notification') },
  { name: 'powerMonitor', loader: () => require('./power-monitor') },
  { name: 'powerSaveBlocker', loader: () => require('./power-save-blocker') },
  { name: 'protocol', loader: () => require('./protocol') },
  { name: 'safeStorage', loader: () => require('./safe-storage') },
  { name: 'screen', loader: () => require('./screen') },
  { name: 'ScrollView', loader: () => require('./scroll-view') },
  { name: 'session', loader: () => require('./session') },
  { name: 'ShareMenu', loader: () => require('./share-menu') },
  { name: 'systemPreferences', loader: () => require('./system-preferences') },
  { name: 'TouchBar', loader: () => require('./touch-bar') },
  { name: 'Tray', loader: () => require('./tray') },
  { name: 'View', loader: () => require('./view') },
  { name: 'webContents', loader: () => require('./web-contents') },
  { name: 'WebContentsView', loader: () => require('./web-contents-view') },
  { name: 'webFrameMain', loader: () => require('./web-frame-main') }
];

if (BUILDFLAG(ENABLE_DESKTOP_CAPTURER)) {
  browserModuleList.push(
    { name: 'desktopCapturer', loader: () => require('./desktop-capturer') }
  );
}

if (BUILDFLAG(ENABLE_VIEWS_API)) {
  browserModuleList.push(
    { name: 'ImageView', loader: () => require('./views/image-view') }
  );
}
