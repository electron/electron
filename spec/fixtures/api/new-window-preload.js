const { ipcRenderer } = require('electron');

ipcRenderer.send('answer', process.argv);
window.close();
