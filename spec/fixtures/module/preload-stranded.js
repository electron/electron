const { ipcRenderer } = require('electron');

ipcRenderer.send('stranded-document-preload', window.location.href);
