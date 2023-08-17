const { ipcRenderer } = require('electron')

const saveBtn = document.getElementById('save-dialog')

saveBtn.addEventListener('click', event => {
  ipcRenderer.send('save-dialog')
})

ipcRenderer.on('saved-file', (event, path) => {
  if (!path) path = 'No path'
  document.getElementById('file-saved').innerHTML = `Path selected: ${path}`
})
