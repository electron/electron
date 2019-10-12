const {ipcRenderer} = require('electron')

const appInfoBtn = document.getElementById('app-info')
const links = document.querySelectorAll('a[href]')

appInfoBtn.addEventListener('click', () => {
  ipcRenderer.send('get-app-path')
})

ipcRenderer.on('got-app-path', (event, path) => {
  const message = `This app is located at: ${path}`
  document.getElementById('got-app-info').innerHTML = message
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