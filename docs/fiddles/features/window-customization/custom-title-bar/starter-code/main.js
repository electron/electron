const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({})
  win.loadURL('https://example.com')
}

app.whenReady().then(() => {
  createWindow()
})
