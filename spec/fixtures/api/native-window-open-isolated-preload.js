const { ipcRenderer } = require('electron');

window.addEventListener('message', (event) => {
  ipcRenderer.send('answer', event.data);
});
