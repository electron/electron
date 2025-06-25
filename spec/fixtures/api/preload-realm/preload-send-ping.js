const { ipcRenderer } = require('electron');

ipcRenderer.send('ping');
