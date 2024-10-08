const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({
    width: 300,
    height: 200,
    frame: false
  })
  win.loadURL('https://example.com')
}

app.whenReady().then(() => {
  createWindow()
})
