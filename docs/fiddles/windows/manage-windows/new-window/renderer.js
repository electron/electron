const { ipcRenderer } = require('electron/renderer')

const newWindowBtn = document.getElementById('new-window')

newWindowBtn.addEventListener('click', (event) => {
  const url = 'https://electronjs.org'
  ipcRenderer.send('new-window', { url, width: 400, height: 320 })
})
