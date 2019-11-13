const { ipcRenderer, shell } = require('electron')

const selectDirBtn = document.getElementById('select-directory')
const links = document.querySelectorAll('a[href]')

selectDirBtn.addEventListener('click', event => {
  ipcRenderer.send('open-file-dialog')
})

ipcRenderer.on('selected-directory', (event, path) => {
  document.getElementById('selected-file').innerHTML = `You selected: ${path}`
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
