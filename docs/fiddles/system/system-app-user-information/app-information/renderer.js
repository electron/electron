const { ipcRenderer } = require('electron/renderer')

const appInfoBtn = document.getElementById('app-info')

appInfoBtn.addEventListener('click', async () => {
  const path = await ipcRenderer.invoke('get-app-path')
  const message = `This app is located at: ${path}`
  document.getElementById('got-app-info').innerHTML = message
})
