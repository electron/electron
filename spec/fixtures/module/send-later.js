const { ipcRenderer } = require('electron');
window.onload = function () {
  ipcRenderer.send('answer', typeof window.process, typeof window.Buffer);
};
