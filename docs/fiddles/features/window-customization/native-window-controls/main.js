const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({
    // remove the default titlebar
    titleBarStyle: 'hidden',
    // expose window controlls in Windows/Linux
    titleBarOverlay: true
  })
  win.loadURL('https://electronjs.org')
}

app.whenReady().then(() => {
  createWindow()
})
