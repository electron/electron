const { app, BrowserWindow, ipcMain } = require('electron')

ipcMain.on('create-frameless-window', (event, {url}) => {
  const win = new BrowserWindow({ frame: false })
  win.loadURL(url)
})

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 600,
    height: 400,
    title: 'Create a frameless window',
    webPreferences: {
      nodeIntegration: true
    }
  })
  mainWindow.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()
})
