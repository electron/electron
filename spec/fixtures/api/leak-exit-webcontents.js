const { app, webContents } = require('electron')
app.on('ready', function () {
  webContents.create({})

  app.quit()
})
