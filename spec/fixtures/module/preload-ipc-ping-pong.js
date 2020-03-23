const { ipcRenderer } = require('electron');

ipcRenderer.on('ping', function (event, payload) {
  ipcRenderer.sendTo(event.senderId, 'pong', payload);
});

ipcRenderer.on('ping-æøåü', function (event, payload) {
  ipcRenderer.sendTo(event.senderId, 'pong-æøåü', payload);
});
