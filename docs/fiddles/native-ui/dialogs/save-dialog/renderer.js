const { ipcRenderer } = require('electron/renderer')

const saveBtn = document.getElementById('save-dialog')

saveBtn.addEventListener('click', event => {
  ipcRenderer.send('save-dialog')
})

ipcRenderer.on('saved-file', (event, path) => {
  if (!path) path = 'No path'
  document.getElementById('file-saved').innerHTML = `Path selected: ${path}`
})
