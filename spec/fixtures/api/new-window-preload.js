const { ipcRenderer } = require('electron');

ipcRenderer.send('answer', {
  argv: process.argv
});
window.close();
