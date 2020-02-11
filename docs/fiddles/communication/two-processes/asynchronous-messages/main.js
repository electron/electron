const { app, BrowserWindow, ipcMain } = require('electron')

let mainWindow = null

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 400,
    title: 'Asynchronous messages',
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

ipcMain.on('asynchronous-message', (event, arg) => {
  event.sender.send('asynchronous-reply', 'pong')
})
