const roles = {
  undo: {
    label: 'Undo',
    accelerator: 'CmdOrCtrl+Z',
    method: 'undo'
  },
  redo: {
    label: 'Redo',
    accelerator: 'Shift+CmdOrCtrl+Z',
    method: 'redo'
  },
  cut: {
    label: 'Cut',
    accelerator: 'CmdOrCtrl+X',
    method: 'cut'
  },
  copy: {
    label: 'Copy',
    accelerator: 'CmdOrCtrl+C',
    method: 'copy'
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
  selectall: {
    label: 'Select All',
    accelerator: 'CmdOrCtrl+A',
    method: 'selectAll'
  },
  minimize: {
    label: 'Minimize',
    accelerator: 'CmdOrCtrl+M',
    method: 'minimize'
  },
  close: {
    label: 'Close',
    accelerator: 'CmdOrCtrl+W',
    method: 'close'
  },
  delete: {
    label: 'Delete',
    accelerator: 'Delete',
    method: 'delete'
  },
  quit: {
    get label () {
      const {app} = require('electron')
      return process.platform === 'win32' ? 'Exit' : `Quit ${app.getName()}`
    },
    accelerator: process.platform === 'win32' ? null : 'Command+Q',
    method: 'quit'
  },
  togglefullscreen: {
    label: 'Toggle Full Screen',
    accelerator: process.platform === 'darwin' ? 'Ctrl+Command+F' : 'F11',
    method: 'toggleFullScreen'
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
