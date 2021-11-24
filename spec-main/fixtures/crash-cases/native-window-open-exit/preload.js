const { ipcRenderer } = require('electron');

window.addEventListener('DOMContentLoaded', () => {
  window.open('127.0.0.1:7001', '_blank');
  setTimeout(() => ipcRenderer.send('test-done'));
});
