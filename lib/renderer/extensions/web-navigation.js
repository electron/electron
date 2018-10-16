'use strict'

const Event = require('@electron/internal/renderer/extensions/event')
const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')

class WebNavigation {
  constructor () {
    this.onBeforeNavigate = new Event()
    this.onCompleted = new Event()

    ipcRenderer.on('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', (event, details) => {
      this.onBeforeNavigate.emit(details)
    })

    ipcRenderer.on('CHROME_WEBNAVIGATION_ONCOMPLETED', (event, details) => {
      this.onCompleted.emit(details)
    })
  }
}

exports.setup = () => {
  return new WebNavigation()
}
