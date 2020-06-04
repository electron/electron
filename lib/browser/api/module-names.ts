// TODO: Figure out a way to not duplicate this information between here and module-list
// It is currently duplicated as module-list "require"s all the browser API file and the
// remote module in the renderer process depends on that file.  As a result webpack
// includes all the browser API files in the renderer process as well and we want to avoid that

// Browser side modules, please sort alphabetically.
export const browserModuleNames = [
  'app',
  'autoUpdater',
  'BrowserView',
  'BrowserWindow',
  'contentTracing',
  'crashReporter',
  'dialog',
  'globalShortcut',
  'ipcMain',
  'inAppPurchase',
  'Menu',
  'MenuItem',
  'nativeTheme',
  'net',
  'netLog',
  'MessageChannelMain',
  'Notification',
  'powerMonitor',
  'powerSaveBlocker',
  'protocol',
  'screen',
  'session',
  'systemPreferences',
  'TopLevelWindow',
  'TouchBar',
  'Tray',
  'View',
  'webContents',
  'WebContentsView'
];

if (BUILDFLAG(ENABLE_DESKTOP_CAPTURER)) {
  browserModuleNames.push('desktopCapturer');
}

if (BUILDFLAG(ENABLE_VIEWS_API)) {
  browserModuleNames.push(
    'ImageView'
  );
}
