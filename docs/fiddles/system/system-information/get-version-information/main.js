const { app, BrowserWindow, shell } = require('electron/main')
const path = require('node:path')

let mainWindow = null

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 400,
    title: 'Get version information',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  }

  mainWindow = new BrowserWindow(windowOptions)
  mainWindow.loadFile('index.html')

  mainWindow.on('closed', () => {
    mainWindow = null
  })

  // Open external links in the default browser
  mainWindow.webContents.on('will-navigate', (event, url) => {
    event.preventDefault()
    shell.openExternal(url)
  })
}

app.whenReady().then(() => {
  createWindow()
})
