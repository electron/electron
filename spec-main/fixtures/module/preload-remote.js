const { ipcRenderer, remote } = require('electron')

window.onload = function () {
  ipcRenderer.send('remote', typeof remote)
}
