const { ipcRenderer } = require('electron')
window.answer = function (answer) {
  ipcRenderer.send('answer', answer)
}
