const roles = {
  about: {
    get label () {
      const {app} = require('electron')
      return `About ${app.getName()}`
    }
  },
  close: {
    label: 'Close',
    accelerator: 'CmdOrCtrl+W',
    method: 'close'
  },
  copy: {
    label: 'Copy',
    accelerator: 'CmdOrCtrl+C',
    method: 'copy'
  },
  cut: {
    label: 'Cut',
    accelerator: 'CmdOrCtrl+X',
    method: 'cut'
  },
  delete: {
    label: 'Delete',
    accelerator: 'Delete',
    method: 'delete'
  },
  front: {
    label: 'Bring All to Front'
  },
  help: {
    label: 'Help'
  },
  hide: {
    get label () {
      const {app} = require('electron')
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
    accelerator: 'CmdOrCtrl+M',
    method: 'minimize'
  },
  paste: {
    label: 'Paste',
    accelerator: 'CmdOrCtrl+V',
    method: 'paste'
  },
  pasteandmatchstyle: {
    label: 'Paste and Match Style',
    accelerator: 'Shift+Command+V',
    method: 'pasteAndMatchStyle'
  },
  quit: {
    get label () {
      const {app} = require('electron')
      return process.platform === 'win32' ? 'Exit' : `Quit ${app.getName()}`
    },
    accelerator: process.platform === 'win32' ? null : 'Command+Q',
    method: 'quit'
  },
  redo: {
    label: 'Redo',
    accelerator: 'Shift+CmdOrCtrl+Z',
    method: 'redo'
  },
  selectall: {
    label: 'Select All',
    accelerator: 'CmdOrCtrl+A',
    method: 'selectAll'
  },
  services: {
    label: 'Services'
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: process.platform === 'darwin' ? 'Ctrl+Command+F' : 'F11',
    method: 'toggleFullScreen'
  },
  undo: {
    label: 'Undo',
    accelerator: 'CmdOrCtrl+Z',
    method: 'undo'
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

exports.getDefaultLabel = function (role) {
  if (roles.hasOwnProperty(role)) {
    return roles[role].label
  } else {
    return ''
  }
}

exports.getDefaultAccelerator = function (role) {
  if (roles.hasOwnProperty(role)) return roles[role].accelerator
}
