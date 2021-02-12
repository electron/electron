const { shell, ipcRenderer } = require('electron')

const newWindowBtn = document.getElementById('new-window')
const link = document.getElementById('browser-window-link')

newWindowBtn.addEventListener('click', (event) => {
  const url = 'https://electronjs.org'
  ipcRenderer.send('new-window', { url, width: 400, height: 320 });
})

link.addEventListener('click', (e) => {
  e.preventDefault()
  shell.openExternal("https://electronjs.org/docs/api/browser-window")
})
