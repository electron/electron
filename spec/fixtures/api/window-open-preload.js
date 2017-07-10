const {ipcRenderer} = require('electron')

setImmediate(function () {
  if (window.location.toString() === 'bar://page') {
    ipcRenderer.send('answer', process.argv, typeof global.process)
    window.close()
  }
})
