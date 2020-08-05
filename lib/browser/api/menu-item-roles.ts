import { app, BrowserWindow, WebContents, MenuItemConstructorOptions } from 'electron/main';

const isMac = process.platform === 'darwin';
const isWindows = process.platform === 'win32';
const isLinux = process.platform === 'linux';

type RoleId = 'about' | 'close' | 'copy' | 'cut' | 'delete' | 'forcereload' | 'front' | 'help' | 'hide' | 'hideothers' | 'minimize' |
  'paste' | 'pasteandmatchstyle' | 'quit' | 'redo' | 'reload' | 'resetzoom' | 'selectall' | 'services' | 'recentdocuments' | 'clearrecentdocuments' | 'startspeaking' | 'stopspeaking' |
  'toggledevtools' | 'togglefullscreen' | 'undo' | 'unhide' | 'window' | 'zoom' | 'zoomin' | 'zoomout' | 'appmenu' | 'filemenu' | 'editmenu' | 'viewmenu' | 'windowmenu'
interface Role {
  label: string;
  accelerator?: string;
  windowMethod?: ((window: BrowserWindow) => void);
  webContentsMethod?: ((webContents: WebContents) => void);
  appMethod?: () => void;
  registerAccelerator?: boolean;
  nonNativeMacOSRole?: boolean;
  submenu?: MenuItemConstructorOptions[];
}

export const roleList: Record<RoleId, Role> = {
  about: {
    get label () {
      return isLinux ? 'About' : `About ${app.name}`;
    },
    ...(isWindows && { appMethod: () => app.showAboutPanel() })
  },
  close: {
    label: isMac ? 'Close Window' : 'Close',
    accelerator: 'CommandOrControl+W',
    windowMethod: w => w.close()
  },
  copy: {
    label: 'Copy',
    accelerator: 'CommandOrControl+C',
    webContentsMethod: wc => wc.copy(),
    registerAccelerator: false
  },
  cut: {
    label: 'Cut',
    accelerator: 'CommandOrControl+X',
    webContentsMethod: wc => wc.cut(),
    registerAccelerator: false
  },
  delete: {
    label: 'Delete',
    webContentsMethod: wc => wc.delete()
  },
  forcereload: {
    label: 'Force Reload',
    accelerator: 'Shift+CmdOrCtrl+R',
    nonNativeMacOSRole: true,
    windowMethod: (window: BrowserWindow) => {
      window.webContents.reloadIgnoringCache();
    }
  },
  front: {
    label: 'Bring All to Front'
  },
  help: {
    label: 'Help'
  },
  hide: {
    get label () {
      return `Hide ${app.name}`;
    },
    accelerator: 'Command+H'
  },
  hideothers: {
    label: 'Hide Others',
    accelerator: 'Command+Alt+H'
  },
  minimize: {
    label: 'Minimize',
    accelerator: 'CommandOrControl+M',
    windowMethod: w => w.minimize()
  },
  paste: {
    label: 'Paste',
    accelerator: 'CommandOrControl+V',
    webContentsMethod: wc => wc.paste(),
    registerAccelerator: false
  },
  pasteandmatchstyle: {
    label: 'Paste and Match Style',
    accelerator: isMac ? 'Cmd+Option+Shift+V' : 'Shift+CommandOrControl+V',
    webContentsMethod: wc => wc.pasteAndMatchStyle(),
    registerAccelerator: false
  },
  quit: {
    get label () {
      switch (process.platform) {
        case 'darwin': return `Quit ${app.name}`;
        case 'win32': return 'Exit';
        default: return 'Quit';
      }
    },
    accelerator: isWindows ? undefined : 'CommandOrControl+Q',
    appMethod: () => app.quit()
  },
  redo: {
    label: 'Redo',
    accelerator: isWindows ? 'Control+Y' : 'Shift+CommandOrControl+Z',
    webContentsMethod: wc => wc.redo()
  },
  reload: {
    label: 'Reload',
    accelerator: 'CmdOrCtrl+R',
    nonNativeMacOSRole: true,
    windowMethod: w => w.reload()
  },
  resetzoom: {
    label: 'Actual Size',
    accelerator: 'CommandOrControl+0',
    nonNativeMacOSRole: true,
    webContentsMethod: (webContents: WebContents) => {
      webContents.zoomLevel = 0;
    }
  },
  selectall: {
    label: 'Select All',
    accelerator: 'CommandOrControl+A',
    webContentsMethod: wc => wc.selectAll()
  },
  services: {
    label: 'Services'
  },
  recentdocuments: {
    label: 'Open Recent'
  },
  clearrecentdocuments: {
    label: 'Clear Menu'
  },
  startspeaking: {
    label: 'Start Speaking'
  },
  stopspeaking: {
    label: 'Stop Speaking'
  },
  toggledevtools: {
    label: 'Toggle Developer Tools',
    accelerator: isMac ? 'Alt+Command+I' : 'Ctrl+Shift+I',
    nonNativeMacOSRole: true,
    windowMethod: w => w.webContents.toggleDevTools()
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: isMac ? 'Control+Command+F' : 'F11',
    windowMethod: (window: BrowserWindow) => {
      window.setFullScreen(!window.isFullScreen());
    }
  },
  undo: {
    label: 'Undo',
    accelerator: 'CommandOrControl+Z',
    webContentsMethod: wc => wc.undo()
  },
  unhide: {
    label: 'Show All'
  },
  window: {
    label: 'Window'
  },
  zoom: {
    label: 'Zoom'
  },
  zoomin: {
    label: 'Zoom In',
    accelerator: 'CommandOrControl+Plus',
    nonNativeMacOSRole: true,
    webContentsMethod: (webContents: WebContents) => {
      webContents.zoomLevel += 0.5;
    }
  },
  zoomout: {
    label: 'Zoom Out',
    accelerator: 'CommandOrControl+-',
    nonNativeMacOSRole: true,
    webContentsMethod: (webContents: WebContents) => {
      webContents.zoomLevel -= 0.5;
    }
  },
  // App submenu should be used for Mac only
  appmenu: {
    get label () {
      return app.name;
    },
    submenu: [
      { role: 'about' },
      { type: 'separator' },
      { role: 'services' },
      { type: 'separator' },
      { role: 'hide' },
      { role: 'hideOthers' },
      { role: 'unhide' },
      { type: 'separator' },
      { role: 'quit' }
    ]
  },
  // File submenu
  filemenu: {
    label: 'File',
    submenu: [
      isMac ? { role: 'close' } : { role: 'quit' }
    ]
  },
  // Edit submenu
  editmenu: {
    label: 'Edit',
    submenu: [
      { role: 'undo' },
      { role: 'redo' },
      { type: 'separator' },
      { role: 'cut' },
      { role: 'copy' },
      { role: 'paste' },
      ...(isMac ? [
        { role: 'pasteAndMatchStyle' },
        { role: 'delete' },
        { role: 'selectAll' },
        { type: 'separator' },
        {
          label: 'Speech',
          submenu: [
            { role: 'startSpeaking' },
            { role: 'stopSpeaking' }
          ]
        }
      ] as MenuItemConstructorOptions[] : [
        { role: 'delete' },
        { type: 'separator' },
        { role: 'selectAll' }
      ] as MenuItemConstructorOptions[])
    ]
  },
  // View submenu
  viewmenu: {
    label: 'View',
    submenu: [
      { role: 'reload' },
      { role: 'forceReload' },
      { role: 'toggleDevTools' },
      { type: 'separator' },
      { role: 'resetZoom' },
      { role: 'zoomIn' },
      { role: 'zoomOut' },
      { type: 'separator' },
      { role: 'togglefullscreen' }
    ]
  },
  // Window submenu
  windowmenu: {
    label: 'Window',
    submenu: [
      { role: 'minimize' },
      { role: 'zoom' },
      ...(isMac ? [
        { type: 'separator' },
        { role: 'front' }
      ] as MenuItemConstructorOptions[] : [
        { role: 'close' }
      ] as MenuItemConstructorOptions[])
    ]
  }
};

const canExecuteRole = (role: keyof typeof roleList) => {
  if (!Object.prototype.hasOwnProperty.call(roleList, role)) return false;
  if (!isMac) return true;

  // macOS handles all roles natively except for a few
  return roleList[role].nonNativeMacOSRole;
};

export function getDefaultLabel (role: RoleId) {
  return Object.prototype.hasOwnProperty.call(roleList, role) ? roleList[role].label : '';
}

export function getDefaultAccelerator (role: RoleId) {
  if (Object.prototype.hasOwnProperty.call(roleList, role)) return roleList[role].accelerator;
}

export function shouldRegisterAccelerator (role: RoleId) {
  const hasRoleRegister = Object.prototype.hasOwnProperty.call(roleList, role) && roleList[role].registerAccelerator !== undefined;
  return hasRoleRegister ? roleList[role].registerAccelerator : true;
}

export function getDefaultSubmenu (role: RoleId) {
  if (!Object.prototype.hasOwnProperty.call(roleList, role)) return;

  let { submenu } = roleList[role];

  // remove null items from within the submenu
  if (Array.isArray(submenu)) {
    submenu = submenu.filter((item) => item != null);
  }

  return submenu;
}

export function execute (role: RoleId, focusedWindow: BrowserWindow, focusedWebContents: WebContents) {
  if (!canExecuteRole(role)) return false;

  const { appMethod, webContentsMethod, windowMethod } = roleList[role];

  if (appMethod) {
    appMethod();
    return true;
  }

  if (windowMethod && focusedWindow != null) {
    windowMethod(focusedWindow);
    return true;
  }

  if (webContentsMethod && focusedWebContents != null) {
    webContentsMethod(focusedWebContents);
    return true;
  }

  return false;
}
