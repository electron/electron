const OUT_DIR = process.env.ELECTRON_OUT_DIR || 'Debug'

const path = require('path')

function getElectronExec () {
  switch (process.platform) {
    case 'darwin':
      return `out/${OUT_DIR}/Electron.app/Contents/MacOS/Electron`
    case 'win32':
      return `out/${OUT_DIR}/electron.exe`
    case 'linux':
      return `out/${OUT_DIR}/electron`
    default:
      throw new Error('Unknown platform')
  }
}

function getAbsoluteElectronExec () {
  return path.resolve(__dirname, '../../..', getElectronExec())
}

module.exports = {
  getElectronExec,
  getAbsoluteElectronExec,
  OUT_DIR
}
