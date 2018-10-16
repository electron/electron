const { app, ipcMain } = require('electron')

app.on('ready', () => {
  process.stdout.write(JSON.stringify(ipcMain.eventNames()))
  process.stdout.end()

  setImmediate(() => {
    app.quit()
  })
})
