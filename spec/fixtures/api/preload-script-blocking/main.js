const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('path')

process.on('uncaughtException', (error) => {
  console.error(error)
  process.exit(1)
})

// simulate bundled app
app.setAppPath(path.resolve(process.resourcesPath, 'app.asar'))

const sandbox = app.commandLine.hasSwitch('sandbox')

app.on('ready', async () => {
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.resolve(__dirname, 'preload.js'),
      sandbox
    }
  })

  mainWindow.webContents.on('preload-error', (event, preloadPath, error) => {
    console.log(JSON.stringify(true))
    app.quit()
  })

  await mainWindow.loadURL('about:blank')

  console.log(JSON.stringify(false))
  app.quit()
})
