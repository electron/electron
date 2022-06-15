const { ipcRenderer } = require('electron');

window.setTimeout(async (_) => {
  document.body.style.background = 'transparent';

  window.setTimeout(async (_) => {
    ipcRenderer.send('set-transparent');
  }, 2000);
}, 3000);
