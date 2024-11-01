const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({
    width: 100,
    height: 100,
    resizable: false,
    frame: false,
    transparent: true
  })
  win.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()
})
