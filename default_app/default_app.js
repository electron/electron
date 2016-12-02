const {app, BrowserWindow, crashReporter} = require('electron')
const path = require('path')

let mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    const options = {
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true
    }
    if (process.platform === 'linux') {
      options.icon = path.join(__dirname, 'icon.png')
    }
    crashReporter.start({
      submitURL: "http://localhost:8080/uploadDump/mainDump",
      companyName: "Main Company",
      productName: "Main Product"
    })

    mainWindow = new BrowserWindow(options)
    mainWindow.loadURL(appUrl)
    mainWindow.focus()
  })
}
