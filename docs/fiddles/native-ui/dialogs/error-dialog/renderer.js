const { ipcRenderer, shell } = require('electron')

const links = document.querySelectorAll('a[href]')
const errorBtn = document.getElementById('error-dialog')

errorBtn.addEventListener('click', event => {
  ipcRenderer.send('open-error-dialog')
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