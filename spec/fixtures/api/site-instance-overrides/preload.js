const { ipcRenderer } = require('electron');

ipcRenderer.send('pid', process.pid);
