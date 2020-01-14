const { app, BrowserWindow } = require('electron')

let mainWindow = null

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 400,
    title: 'Clipboard paste',
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

app.on('ready', () => {
  createWindow()
})
