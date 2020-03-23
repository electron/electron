const { ipcRenderer } = require('electron');
window.onload = function () {
  ipcRenderer.send('answer', process.argv);
};
