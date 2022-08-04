const { ipcRenderer } = require('electron');

window.addEventListener('message', (event) => {
  ipcRenderer.send('leak-result', event.data);
});
