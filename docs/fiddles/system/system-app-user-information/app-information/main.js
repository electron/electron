const {app, ipcMain} = require('electron')

ipcMain.on('get-app-path', (event) => {
  event.sender.send('got-app-path', app.getAppPath())
})