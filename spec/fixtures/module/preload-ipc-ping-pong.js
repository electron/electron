const { ipcRenderer } = require('electron')
ipcRenderer.on('ping', function (event, payload) {
  ipcRenderer.sendTo(event.senderId, 'pong', payload)
})
