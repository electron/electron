const { ipcRenderer, webFrame } = require('electron');

window.isolatedGlobal = true;

ipcRenderer.send('preload-ran', window.location.href, webFrame.frameToken);

ipcRenderer.on('preload-ping', () => {
  ipcRenderer.send('preload-pong', webFrame.frameToken);
});

window.addEventListener('unload', () => {
  ipcRenderer.send('preload-unload', window.location.href);
});
