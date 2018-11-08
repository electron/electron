const { app, webContents } = require('electron')
app.on('ready', function () {
  webContents.create({})

  process.nextTick(() => app.quit())
})
