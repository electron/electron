const { ipcRenderer } = require('electron');

ipcRenderer.on('ping', () => {
  ipcRenderer.send('pong');
});
