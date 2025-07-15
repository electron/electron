const { ipcRenderer } = require('electron/renderer')

document.addEventListener('DOMContentLoaded', () => {
  const textarea = document.getElementById('editable')
  // highlight-start
  textarea.addEventListener('contextmenu', (event) => {
    event.preventDefault()
    ipcRenderer.send('context-menu')
  })
  // highlight-end
})
