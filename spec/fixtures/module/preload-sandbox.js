(function () {
  const {setImmediate} = require('timers')
  const {ipcRenderer} = require('electron')
  window.ipcRenderer = ipcRenderer
  window.setImmediate = setImmediate
  window.require = require

  process.once('loaded', () => {
    ipcRenderer.send('process-loaded')
  })

  if (location.protocol === 'file:') {
    window.test = 'preload'
    window.process = process
    if (process.env.sandboxmain) {
      window.test = {
        env: process.env,
        execPath: process.execPath,
        platform: process.platform
      }
    }
  } else if (location.href !== 'about:blank') {
    addEventListener('DOMContentLoaded', () => {
      ipcRenderer.send('child-loaded', window.opener == null, document.body.innerHTML, location.href)
    }, false)
  }
})()
