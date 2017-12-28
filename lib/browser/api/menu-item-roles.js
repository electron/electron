const {app} = require('electron')

const roles = {
  about: {
    get label () {
      return process.platform === 'linux' ? 'About' : `About ${app.getName()}`
    }
  },
  close: {
    label: process.platform === 'darwin' ? 'Close Window' : 'Close',
    accelerator: 'CommandOrControl+W',
    windowMethod: 'close'
  },
  copy: {
    label: 'Copy',
    accelerator: 'CommandOrControl+C',
    webContentsMethod: 'copy'
  },
  cut: {
    label: 'Cut',
    accelerator: 'CommandOrControl+X',
    webContentsMethod: 'cut'
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
    webContentsMethod: 'paste'
  },
  pasteandmatchstyle: {
    label: 'Paste and Match Style',
    accelerator: 'Shift+CommandOrControl+V',
    webContentsMethod: 'pasteAndMatchStyle'
  },
  quit: {
    get label () {
      switch (process.platform) {
        case 'darwin': return `Quit ${app.getName()}`
        case 'win32': return 'Exit'
        default: return 'Quit'
      }
    },
    accelerator: process.platform === 'win32' ? null : 'CommandOrControl+Q',
    appMethod: 'quit'
  },
  redo: {
    label: 'Redo',
    accelerator: process.platform === 'win32' ? 'Control+Y' : 'Shift+CommandOrControl+Z',
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
    accelerator: process.platform === 'darwin' ? 'Alt+Command+I' : 'Ctrl+Shift+I',
    nonNativeMacOSRole: true,
    windowMethod: 'toggleDevTools'
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: process.platform === 'darwin' ? 'Control+Command+F' : 'F11',
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
      webContents.getZoomLevel((zoomLevel) => {
        webContents.setZoomLevel(zoomLevel + 0.5)
      })
    }
  },
  zoomout: {
    label: 'Zoom Out',
    accelerator: 'CommandOrControl+-',
    nonNativeMacOSRole: true,
    webContentsMethod: (webContents) => {
      webContents.getZoomLevel((zoomLevel) => {
        webContents.setZoomLevel(zoomLevel - 0.5)
      })
    }
  },
  // Edit submenu (should fit both Mac & Windows)
  editmenu: {
    label: 'Edit',
    submenu: [
      {
        role: 'undo'
      },
      {
        role: 'redo'
      },
      {
        type: 'separator'
      },
      {
        role: 'cut'
      },
      {
        role: 'copy'
      },
      {
        role: 'paste'
      },

      process.platform === 'darwin' ? {
        role: 'pasteAndMatchStyle'
      } : null,

      {
        role: 'delete'
      },

      process.platform === 'win32' ? {
        type: 'separator'
      } : null,

      {
        role: 'selectAll'
      }
    ]
  },

  // Window submenu should be used for Mac only
  windowmenu: {
    label: 'Window',
    submenu: [
      {
        role: 'minimize'
      },
      {
        role: 'close'
      },

      process.platform === 'darwin' ? {
        type: 'separator'
      } : null,

      process.platform === 'darwin' ? {
        role: 'front'
      } : null

    ]
  }
}

const canExecuteRole = (role) => {
  if (!roles.hasOwnProperty(role)) return false
  if (process.platform !== 'darwin') return true
  // macOS handles all roles natively except for a few
  return roles[role].nonNativeMacOSRole
}

exports.getDefaultLabel = (role) => {
  if (roles.hasOwnProperty(role)) {
    return roles[role].label
  } else {
    return ''
  }
}

exports.getDefaultAccelerator = (role) => {
  if (roles.hasOwnProperty(role)) return roles[role].accelerator
}

exports.getDefaultSubmenu = (role) => {
  if (!roles.hasOwnProperty(role)) return

  let {submenu} = roles[role]

  // remove null items from within the submenu
  if (Array.isArray(submenu)) {
    submenu = submenu.filter((item) => item != null)
  }

  return submenu
}

exports.execute = (role, focusedWindow, focusedWebContents) => {
  if (!canExecuteRole(role)) return false

  const {appMethod, webContentsMethod, windowMethod} = roles[role]

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
