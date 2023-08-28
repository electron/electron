const { BrowserWindow, app, screen, ipcMain, desktopCapturer } = require('electron/main')

let mainWindow = null

ipcMain.handle('get-screen-size', () => {
  return screen.getPrimaryDisplay().workAreaSize
})

ipcMain.handle('get-sources', (event, options) => {
  return desktopCapturer.getSources(options)
})

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 300,
    title: 'Take a Screenshot',
    webPreferences: {
      contextIsolation: false,
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
