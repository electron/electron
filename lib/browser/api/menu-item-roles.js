const {app} = require('electron')

const roles = {
  about: {
    get label () {
      return process.platform === 'linux' ? 'About' : `About ${app.getName()}`
    }
  },
  close: {
    label: 'Close',
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
    accelerator: process.platform === 'win32' ? null : 'Command+Q',
    appMethod: 'quit'
  },
  redo: {
    label: 'Redo',
    accelerator: 'Shift+CommandOrControl+Z',
    webContentsMethod: 'redo'
  },
  selectall: {
    label: 'Select All',
    accelerator: 'CommandOrControl+A',
    webContentsMethod: 'selectAll'
  },
  services: {
    label: 'Services'
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: process.platform === 'darwin' ? 'Ctrl+Command+F' : 'F11',
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
  }
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

exports.execute = (role, focusedWindow) => {
  if (!roles.hasOwnProperty(role)) return false
  if (process.platform === 'darwin') return false

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

  if (webContentsMethod && focusedWindow != null) {
    const {webContents} = focusedWindow
    if (webContents) {
      webContents[webContentsMethod]()
    }
    return true
  }

  return false
}
