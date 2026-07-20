const { ipcRenderer } = require('electron');

ipcRenderer.send('window-open-reuse-preload-ran', window.location.href);
