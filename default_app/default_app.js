const {app, BrowserWindow} = require('electron')
const path = require('path')

let mainWindow = null

app.commandLine.appendSwitch('--disable-gpu');
app.commandLine.appendSwitch('--disable-gpu-compositing');

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    mainWindow = new BrowserWindow({
      width: 800,
      height: 600,
      show: false,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true,
      webPreferences: {
        offscreen: true,
        nodeIntegration: false,
        backgroundThrottling: false
      }
    })
    mainWindow.loadURL(appUrl)
    mainWindow.focus()

    mainWindow.webContents.on('dom-ready', () => {
      mainWindow.webContents.setFrameRate(10)
      let ping = true
      setInterval(() => {
        if (ping) {
          mainWindow.webContents.startPainting()
        } else {
          mainWindow.webContents.stopPainting()
        }

        ping = !ping
      }, 3000)
    })

    setInterval(() => {
      console.log(mainWindow.webContents.isPainting())
    }, 500)

    mainWindow.webContents.on('paint', (e, rect, data) => {
      console.log('painting', data.length)
    })
  })
}
