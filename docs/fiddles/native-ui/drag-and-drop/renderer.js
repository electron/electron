const { ipcRenderer } = require('electron')
const shell = require('electron').shell

const links = document.querySelectorAll('a[href]')

Array.prototype.forEach.call(links, (link) => {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
})

const dragFileLink = document.getElementById('drag-file-link')

dragFileLink.addEventListener('dragstart', event => {
  event.preventDefault()
  ipcRenderer.send('ondragstart', __filename)
})
