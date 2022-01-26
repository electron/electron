const { ipcRenderer } = require('electron');

window.setTimeout(async (_) => {
  document.body.style.background = 'transparent';
}, 3000);

window.setTimeout(async (_) => {
  ipcRenderer.send('set-transparent');
}, 5000);
