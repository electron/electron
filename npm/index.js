var fs = require('fs')
var path = require('path')

var pathFile = path.join(__dirname, 'path.txt')

function getElectronPath () {
  if (fs.existsSync(pathFile)) {
    var executablePath = fs.readFileSync(pathFile, 'utf-8')
    if (process.env.ELECTRON_OVERRIDE_DIST_PATH) {
      return path.join(process.env.ELECTRON_OVERRIDE_DIST_PATH, executablePath)
    }
    return path.join(__dirname, 'dist', executablePath)
  } else {
    throw new Error('Electron failed to install correctly, please delete node_modules/electron and try installing again')
  }
}

// A list of the main modules the people will attempt to use from the Electron API, this is not a complete list but should cover most
// use cases.
var electronModuleNames = ['app', 'autoUpdater', 'BrowserWindow', 'ipcMain', 'Menu', 'net', 'Notification', 'systemPreferences', 'Tray']
var electronPath = new String(getElectronPath())

electronModuleNames.forEach(function warnOnElectronAPIAccess (apiKey) {
  Object.defineProperty(electronPath, apiKey, {
    enumerable: false,
    configurable: false,
    get: function getElectronAPI () {
      console.warn('WARNING: You are attempting to access an Electron API from a node environment.\n\n' +
      'You need to use the "electron" command to run your app. E.g. "electron ./myapp.js"')
    }
  })
})

module.exports = electronPath
