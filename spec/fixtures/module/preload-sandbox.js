(function () {
  const {setImmediate} = require('timers')
  const {ipcRenderer} = require('electron')
  window.ipcRenderer = ipcRenderer
  window.setImmediate = setImmediate
  window.require = require
  if (location.protocol === 'file:') {
    window.test = 'preload'
    window.process = process
  } else if (location.href !== 'about:blank') {
    addEventListener('DOMContentLoaded', () => {
      ipcRenderer.send('child-loaded', window.opener == null, document.body.innerHTML)
    }, false)
  }
})()
