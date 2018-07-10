var ipcRenderer = require('electron').ipcRenderer
window.answer = function (answer) {
  ipcRenderer.send('answer', answer)
}
