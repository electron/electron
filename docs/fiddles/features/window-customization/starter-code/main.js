const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({})
  win.loadURL('https://electronjs.org')
}

app.whenReady().then(() => {
  createWindow()
})
