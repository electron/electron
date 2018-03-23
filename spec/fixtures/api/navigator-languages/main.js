const {app, BrowserWindow, ipcMain} = require('electron')
const path = require('path')

let mainWindow = null

app.on('ready', () => {
  ipcMain.once('nav-lang', (_, langs) => {
    console.log(langs)
    app.quit()
  })

  const options = {
    width: 400,
    height: 400,
    show: false
  }
  mainWindow = new BrowserWindow(options)
  mainWindow.loadURL(`file://${path.join(__dirname, 'index.html')}`)
  mainWindow.focus()
})
