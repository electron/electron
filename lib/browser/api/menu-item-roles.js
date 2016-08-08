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
    accelerator: 'Shift+CommandOrControl+Z',
    webContentsMethod: 'redo'
  },
  resetzoom: {
    label: 'Actual Size',
    accelerator: 'CommandOrControl+0',
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
  startspeaking: {
    label: 'Start Speaking'
  },
  stopspeaking: {
    label: 'Stop Speaking'
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: process.platform === 'darwin' ? 'Control+Command+F' : 'F11',
    windowMethod: function (window) {
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
    webContentsMethod: (webContents) => {
      webContents.getZoomLevel((zoomLevel) => {
        webContents.setZoomLevel(zoomLevel + 0.5)
      })
    }
  },
  zoomout: {
    label: 'Zoom Out',
    accelerator: 'CommandOrControl+-',
    webContentsMethod: (webContents) => {
      webContents.getZoomLevel((zoomLevel) => {
        webContents.setZoomLevel(zoomLevel - 0.5)
      })
    }
  }
}

const canExecuteRole = (role) => {
  if (!roles.hasOwnProperty(role)) return false
  if (process.platform !== 'darwin') return true
  // macOS handles all roles natively except the ones listed below
  return ['zoomin', 'zoomout', 'resetzoom'].includes(role)
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
