const { app, BrowserWindow } = require('electron')

app.once('ready', async () => {
  const mainWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: 'preload.js',
      sandbox: app.commandLine.hasSwitch('sandbox-window')
    }
  })
  await mainWindow.loadURL('about:blank')
  app.exit(1)
})
