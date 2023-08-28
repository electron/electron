const { ipcRenderer } = require('electron/renderer')

const selectDirBtn = document.getElementById('select-directory')

selectDirBtn.addEventListener('click', event => {
  ipcRenderer.send('open-file-dialog')
})

ipcRenderer.on('selected-directory', (event, path) => {
  document.getElementById('selected-file').innerHTML = `You selected: ${path}`
})
