'use strict'

const { app } = require('electron')

const isMac = process.platform === 'darwin'
const isWindows = process.platform === 'win32'
const isLinux = process.platform === 'linux'

const roles = {
  about: {
    get label () {
      return isLinux ? 'About' : `About ${app.getName()}`
    }
  },
  close: {
    label: isMac ? 'Close Window' : 'Close',
    accelerator: 'CommandOrControl+W',
    windowMethod: 'close'
  },
  copy: {
    label: 'Copy',
    accelerator: 'CommandOrControl+C',
    webContentsMethod: 'copy',
    registerAccelerator: false
  },
  cut: {
    label: 'Cut',
    accelerator: 'CommandOrControl+X',
    webContentsMethod: 'cut',
    registerAccelerator: false
  },
  delete: {
    label: 'Delete',
    webContentsMethod: 'delete'
  },
  forcereload: {
    label: 'Force Reload',
    accelerator: 'Shift+CmdOrCtrl+R',
    nonNativeMacOSRole: true,
    windowMethod: (window) => {
      window.webContents.reloadIgnoringCache()
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
      return `Hide ${app.getName()}`
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
    windowMethod: 'minimize'
  },
  paste: {
    label: 'Paste',
    accelerator: 'CommandOrControl+V',
    webContentsMethod: 'paste',
    registerAccelerator: false
  },
  pasteandmatchstyle: {
    label: 'Paste and Match Style',
    accelerator: 'Shift+CommandOrControl+V',
    webContentsMethod: 'pasteAndMatchStyle',
    registerAccelerator: false
  },
  quit: {
    get label () {
      switch (process.platform) {
        case 'darwin': return `Quit ${app.getName()}`
        case 'win32': return 'Exit'
        default: return 'Quit'
      }
    },
    accelerator: isWindows ? null : 'CommandOrControl+Q',
    appMethod: 'quit'
  },
  redo: {
    label: 'Redo',
    accelerator: isWindows ? 'Control+Y' : 'Shift+CommandOrControl+Z',
    webContentsMethod: 'redo'
  },
  reload: {
    label: 'Reload',
    accelerator: 'CmdOrCtrl+R',
    nonNativeMacOSRole: true,
    windowMethod: 'reload'
  },
  resetzoom: {
    label: 'Actual Size',
    accelerator: 'CommandOrControl+0',
    nonNativeMacOSRole: true,
    webContentsMethod: (webContents) => {
      webContents.setZoomLevel(0)
    }
  },
  selectall: {
    label: 'Select All',
    accelerator: 'CommandOrControl+A',
    webContentsMethod: 'selectAll'
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
    windowMethod: 'toggleDevTools'
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: isMac ? 'Control+Command+F' : 'F11',
    windowMethod: (window) => {
      window.setFullScreen(!window.isFullScreen())
    }
  },
  undo: {
    label: 'Undo',
    accelerator: 'CommandOrControl+Z',
    webContentsMethod: 'undo'
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
    webContentsMethod: (webContents) => {
      const zoomLevel = webContents.getZoomLevel()
      webContents.setZoomLevel(zoomLevel + 0.5)
    }
  },
  zoomout: {
    label: 'Zoom Out',
    accelerator: 'CommandOrControl+-',
    nonNativeMacOSRole: true,
    webContentsMethod: (webContents) => {
      const zoomLevel = webContents.getZoomLevel()
      webContents.setZoomLevel(zoomLevel - 0.5)
    }
  },
  // App submenu should be used for Mac only
  appmenu: {
    get label () {
      return app.getName()
    },
    submenu: [
      { role: 'about' },
      { type: 'separator' },
      { role: 'services' },
      { type: 'separator' },
      { role: 'hide' },
      { role: 'hideothers' },
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
            { role: 'startspeaking' },
            { role: 'stopspeaking' }
          ]
        }
      ] : [
        { role: 'delete' },
        { type: 'separator' },
        { role: 'selectAll' }
      ])
    ]
  },
  // View submenu
  viewmenu: {
    label: 'View',
    submenu: [
      { role: 'reload' },
      { role: 'forcereload' },
      { role: 'toggledevtools' },
      { type: 'separator' },
      { role: 'resetzoom' },
      { role: 'zoomin' },
      { role: 'zoomout' },
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
      ] : [
        { role: 'close' }
      ])
    ]
  }
}

const canExecuteRole = (role) => {
  if (!roles.hasOwnProperty(role)) return false
  if (!isMac) return true

  // macOS handles all roles natively except for a few
  return roles[role].nonNativeMacOSRole
}

exports.getDefaultLabel = (role) => {
  return roles.hasOwnProperty(role) ? roles[role].label : ''
}

exports.getDefaultAccelerator = (role) => {
  if (roles.hasOwnProperty(role)) return roles[role].accelerator
}

exports.shouldRegisterAccelerator = (role) => {
  const hasRoleRegister = roles.hasOwnProperty(role) && roles[role].registerAccelerator !== undefined
  return hasRoleRegister ? roles[role].registerAccelerator : true
}

exports.getDefaultSubmenu = (role) => {
  if (!roles.hasOwnProperty(role)) return

  let { submenu } = roles[role]

  // remove null items from within the submenu
  if (Array.isArray(submenu)) {
    submenu = submenu.filter((item) => item != null)
  }

  return submenu
}

exports.execute = (role, focusedWindow, focusedWebContents) => {
  if (!canExecuteRole(role)) return false

  const { appMethod, webContentsMethod, windowMethod } = roles[role]

  if (appMethod) {
    app[appMethod]()
    return true
  }

  if (windowMethod && focusedWindow != null) {
    if (typeof windowMethod === 'function') {
      windowMethod(focusedWindow)
    } else {
      focusedWindow[windowMethod]()
    }
    return true
  }

  if (webContentsMethod && focusedWebContents != null) {
    if (typeof webContentsMethod === 'function') {
      webContentsMethod(focusedWebContents)
    } else {
      focusedWebContents[webContentsMethod]()
    }
    return true
  }

  return false
}
