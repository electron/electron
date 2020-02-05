const { BrowserWindow, app } = require('electron')

let mainWindow = null

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 300,
    title: 'Basic Notification',
    webPreferences: {
      nodeIntegration: true
    }
  }

  mainWindow = new BrowserWindow(windowOptions)
  mainWindow.loadFile('index.html')

  mainWindow.on('closed', () => {
    mainWindow = null
  })
}

app.whenReady().then(() => {
  createWindow()
})
