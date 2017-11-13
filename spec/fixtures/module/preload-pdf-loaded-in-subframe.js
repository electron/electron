const {ipcRenderer} = require('electron')

document.addEventListener('DOMContentLoaded', (event) => {
  var subframe = document.getElementById('pdf-frame')
  if (subframe) {
    subframe.contentWindow.addEventListener('pdf-loaded', (event) => {
      ipcRenderer.send('pdf-loaded', event.detail)
    })
  }
})
