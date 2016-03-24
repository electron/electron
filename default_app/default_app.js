const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow

var mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', function () {
  app.quit()
})

exports.load = function (appUrl) {
  app.on('ready', function () {
    mainWindow = new BrowserWindow({
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      useContentSize: true,
    })
    mainWindow.loadURL(appUrl)
    mainWindow.focus()
  })
}
