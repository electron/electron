const { ipcRenderer } = require('electron')

document.addEventListener('DOMContentLoaded', (event) => {
  const subframe = document.querySelector('#pdf-frame')
  if (subframe) {
    subframe.contentWindow.addEventListener('pdf-loaded', (event) => {
      ipcRenderer.send('pdf-loaded', event.detail)
    })
  }
})
