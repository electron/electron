const { ipcRenderer } = require('electron')

window.addEventListener('pdf-loaded', function (event) {
  ipcRenderer.send('pdf-loaded', event.detail)
})
