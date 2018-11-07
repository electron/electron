'use strict'

const { ipcRenderer } = require('electron')

function setup (binding) {
  binding.setShutdownHandler(function () {
    ipcRenderer.send('ELECTRON_BROWSER_QUERYENDSESSION')
  })
}

module.exports = setup
