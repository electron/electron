const {ipcRenderer} = require('electron')

const appInfoBtn = document.getElementById('app-info')

appInfoBtn.addEventListener('click', () => {
  ipcRenderer.send('get-app-path')
})

ipcRenderer.on('got-app-path', (event, path) => {
  const message = `This app is located at: ${path}`
  document.getElementById('got-app-info').innerHTML = message
})