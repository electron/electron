const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({
    // remove the default titlebar
    titleBarStyle: 'hidden'
  })
  win.loadURL('https://example.com')
}

app.whenReady().then(() => {
  createWindow()
})
