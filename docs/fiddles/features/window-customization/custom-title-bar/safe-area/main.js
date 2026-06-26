const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({
    // remove the default titlebar
    titleBarStyle: 'hidden',
    // expose window controls in Windows/Linux
    ...(process.platform !== 'darwin' ? { titleBarOverlay: true } : {})
  })

  win.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()
})
