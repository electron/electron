const { ipcRenderer } = require('electron/renderer')

const errorBtn = document.getElementById('error-dialog')

errorBtn.addEventListener('click', event => {
  ipcRenderer.send('open-error-dialog')
})
