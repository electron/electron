const { ipcRenderer } = require('electron');

ipcRenderer.on('port', (e, msg) => {
  e.ports[0].postMessage(msg);
});
