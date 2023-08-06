const { ipcRenderer } = require('electron')

// Tell main process to show the menu when demo button is clicked
const contextMenuBtn = document.getElementById('context-menu')

contextMenuBtn.addEventListener('click', () => {
  ipcRenderer.send('show-context-menu')
})
