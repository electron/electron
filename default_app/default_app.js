const {app, BrowserWindow} = require('electron')

var mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    mainWindow = new BrowserWindow({
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true
    })
    mainWindow.loadURL(appUrl)
    mainWindow.focus()
    mainWindow.webContents.on('dom-ready', () => {
      mainWindow.webContents.beginFrameSubscription(() => {
        console.log("asd")
      })
    })
  })
}
