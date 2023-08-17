const { ipcRenderer } = require('electron')

const dragFileLink = document.getElementById('drag-file-link')

dragFileLink.addEventListener('dragstart', event => {
  event.preventDefault()
  ipcRenderer.send('ondragstart', __filename)
})
