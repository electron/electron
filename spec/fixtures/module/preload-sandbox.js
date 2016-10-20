(function () {
  const {ipcRenderer} = require('electron')
  window.ipcRenderer = ipcRenderer
  if (location.protocol === 'file:') {
    window.test = 'preload'
    window.require = require
    window.process = process
  } else if (location.href !== 'about:blank') {
    addEventListener('DOMContentLoaded', () => {
      ipcRenderer.send('child-loaded', window.opener == null, document.body.innerHTML)
    }, false)
  }
})()
