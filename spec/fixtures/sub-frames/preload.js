const { ipcRenderer, webFrame } = require('electron');

window.isolatedGlobal = true;

ipcRenderer.send('preload-ran', window.location.href, webFrame.routingId);

ipcRenderer.on('preload-ping', () => {
  ipcRenderer.send('preload-pong', webFrame.routingId);
});

window.addEventListener('unload', () => {
  ipcRenderer.send('preload-unload', window.location.href);
});
