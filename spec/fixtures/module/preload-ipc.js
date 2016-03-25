var ipcRenderer = require('electron').ipcRenderer
ipcRenderer.on('ping', function (event, message) {
  ipcRenderer.sendToHost('pong', message)
})
