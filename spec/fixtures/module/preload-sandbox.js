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
        sandboxed: process.sandboxed,
        type: process.type,
        version: process.version,
        versions: process.versions
      }
    }
  } else if (location.href !== 'about:blank') {
    ipcRenderer.once('touch-the-opener', () => {
      let errorMessage = null
      try {
        const openerDoc = opener.document // eslint-disable-line no-unused-vars
      } catch (error) {
        errorMessage = error.message
      }
      ipcRenderer.send('answer', errorMessage)
    })
    ipcRenderer.once('provide-details', () => {
      ipcRenderer.send('answer', window.opener == null, document.body.innerHTML, location.href)
    })
  }
})()
