const { ipcRenderer, shell } = require('electron')

const trayBtn = document.getElementById('put-in-tray')
const links = document.querySelectorAll('a[href]')

let trayOn = false

trayBtn.addEventListener('click', function (event) {
  if (trayOn) {
    trayOn = false
    document.getElementById('tray-countdown').innerHTML = ''
    ipcRenderer.send('remove-tray')
  } else {
    trayOn = true
    const message = 'Click demo again to remove.'
    document.getElementById('tray-countdown').innerHTML = message
    ipcRenderer.send('put-in-tray')
  }
})
// Tray removed from context menu on icon
ipcRenderer.on('tray-removed', function () {
  ipcRenderer.send('remove-tray')
  trayOn = false
  document.getElementById('tray-countdown').innerHTML = ''
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