const { ipcRenderer } = require('electron')

document.addEventListener('DOMContentLoaded', (event) => {
  var outerFrame = document.querySelector('#outer-frame')
  if (outerFrame) {
    outerFrame.onload = function () {
      var pdframe = outerFrame.contentWindow.document.getElementById('pdf-frame')
      if (pdframe) {
        pdframe.contentWindow.addEventListener('pdf-loaded', (event) => {
          ipcRenderer.send('pdf-loaded', event.detail)
        })
      }
    }
  }
})
