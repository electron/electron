var ipcRenderer = require('electron').ipcRenderer
window.onload = function () {
  ipcRenderer.send('answer', typeof window.process, typeof window.Buffer)
}
