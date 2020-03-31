const { ipcRenderer } = require('electron');
ipcRenderer.send('pong');
