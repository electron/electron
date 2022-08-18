const { ipcRenderer, webFrame } = require('electron');

ipcRenderer.send('answer', {
  argv: process.argv
});
window.close();
