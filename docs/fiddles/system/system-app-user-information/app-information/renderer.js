const { ipcRenderer, shell } = require('electron')

const appInfoBtn = document.getElementById('app-info')
const electronDocLink = document.querySelectorAll('a[href]')

appInfoBtn.addEventListener('click', () => {
  ipcRenderer.send('get-app-path')
})

ipcRenderer.on('got-app-path', (event, path) => {
  const message = `This app is located at: ${path}`
  document.getElementById('got-app-info').innerHTML = message
})

electronDocLink.addEventListener('click', (e) => {
  e.preventDefault()
  const url = e.target.getAttribute('href')
  shell.openExternal(url)
})
