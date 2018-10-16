(function () {
  const { setImmediate } = require('timers')
  const { ipcRenderer } = require('electron')
  window.ipcRenderer = ipcRenderer
  window.setImmediate = setImmediate
  window.require = require
  if (location.protocol === 'file:') {
    window.test = 'preload'
    window.process = process
    if (process.env.sandboxmain) {
      window.test = {
        env: process.env,
        execPath: process.execPath,
        pid: process.pid,
        arch: process.arch,
        platform: process.platform,
        resourcesPath: process.resourcesPath,
        sandboxed: process.sandboxed,
        type: process.type,
        version: process.version,
        versions: process.versions
      }
    }
  } else if (location.href !== 'about:blank') {
    addEventListener('DOMContentLoaded', () => {
      ipcRenderer.send('child-loaded', window.opener == null, document.body.innerHTML)
    }, false)
  }
})()
