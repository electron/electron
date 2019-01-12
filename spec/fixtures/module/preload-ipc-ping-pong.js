const { ipcRenderer } = require('electron')

ipcRenderer.on('ping', function (event, senderId, payload) {
  ipcRenderer.sendTo(senderId, 'pong', payload)
})
