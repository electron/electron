'use strict'

const Event = require('@electron/internal/renderer/extensions/event')
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')

class WebNavigation {
  constructor () {
    this.onBeforeNavigate = new Event()
    this.onCompleted = new Event()

    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', (event, details) => {
      this.onBeforeNavigate.emit(details)
    })

    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONCOMPLETED', (event, details) => {
      this.onCompleted.emit(details)
    })
  }
}

exports.setup = () => {
  return new WebNavigation()
}
