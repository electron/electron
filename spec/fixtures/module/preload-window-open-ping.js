// Preload fixture for the window.open() contextIsolation spec. Reports each
// document it runs in and answers pings from the main process from it.
const { ipcRenderer } = require('electron');
ipcRenderer.send('window-open-ping-preload-ran', location.href);
ipcRenderer.on('window-open-ping', () => {
  ipcRenderer.send('window-open-pong', location.href);
});
