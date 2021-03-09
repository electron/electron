const { ipcRenderer } = require('electron');

ipcRenderer.send('context-isolation', process.contextIsolation);
