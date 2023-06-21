const { ipcRenderer } = require('electron');

ipcRenderer.on('ping', function ({ senderId, isMainFrame }, payload) {
  ipcRenderer.sendTo(senderId, 'pong', { payload, isMainFrame });
});

ipcRenderer.on('ping-æøåü', function (event, payload) {
  ipcRenderer.sendTo(event.senderId, 'pong-æøåü', payload);
});
