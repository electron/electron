const { ipcRenderer } = require('electron');

ipcRenderer.on('ping', function (event, message) {
  ipcRenderer.sendToHost('pong', message);
});
