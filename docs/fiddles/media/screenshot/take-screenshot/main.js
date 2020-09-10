const { BrowserWindow, app, screen, ipcMain } = require('electron')

let mainWindow = null

ipcMain.handle('get-screen-size', () => {
  return screen.getPrimaryDisplay().workAreaSize
})

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 300,
    title: 'Take a Screenshot',
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
