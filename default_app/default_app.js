const {app, BrowserWindow, ipcMain} = require('electron')
const path = require('path')

r = require('electron').remote

let mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    const options = {
      width: 900,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      webPreferences: {
        nodeIntegrationInWorker: true
      },
      useContentSize: true
    }
    if (process.platform === 'linux') {
      options.icon = path.join(__dirname, 'icon.png')
    }

    mainWindow = new BrowserWindow(options)
    mainWindow.loadURL(appUrl)
    mainWindow.focus()
    mainWindow.openDevTools()
  })

  ipcMain.on('RUN_IN_MAIN', (event, code) => {
    console.log(code)
    console.log(eval(code))
  })
}
