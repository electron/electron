const { ipcRenderer } = require('electron')

const manageWindowBtn = document.getElementById('manage-window')

ipcRenderer.on('bounds-changed', (event, bounds) => {
  const manageWindowReply = document.getElementById('manage-window-reply')
  const message = `Size: ${bounds.size} Position: ${bounds.position}`
  manageWindowReply.textContent = message
})

manageWindowBtn.addEventListener('click', (event) => {
  ipcRenderer.send('create-demo-window')
})
