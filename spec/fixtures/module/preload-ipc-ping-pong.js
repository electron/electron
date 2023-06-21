const { ipcRenderer } = require('electron');

ipcRenderer.on('ping', function ({ senderId, senderIsMainFrame }, payload) {
  ipcRenderer.sendTo(senderId, 'pong', { payload, senderIsMainFrame });
});

ipcRenderer.on('ping-æøåü', function (event, payload) {
  ipcRenderer.sendTo(event.senderId, 'pong-æøåü', payload);
});
