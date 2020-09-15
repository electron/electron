const { shell, ipcRenderer } = require('electron')

const manageWindowBtn = document.getElementById('manage-window')

const links = document.querySelectorAll('a[href]')

ipcRenderer.on('bounds-changed', (event, bounds) => {
  const manageWindowReply = document.getElementById('manage-window-reply')
  const message = `Size: ${bounds.size} Position: ${bounds.position}`
  manageWindowReply.textContent = message
})

manageWindowBtn.addEventListener('click', (event) => {
  ipcRenderer.send('create-demo-window')
})

Array.prototype.forEach.call(links, (link) => {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
})
