const { app, BrowserWindow } = require('electron')
const path = require('path')

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      sanbox: true
    }
  })

  win.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow()
})
