'use strict'

const { webFrame } = require('electron')
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')
const errorUtils = require('@electron/internal/common/error-utils')

module.exports = () => {
  // Call webFrame method
  ipcRendererInternal.on('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', (event, method, args) => {
    webFrame[method](...args)
  })

  ipcRendererInternal.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
    new Promise(resolve =>
      webFrame[method](...args, resolve)
    ).then(result => {
      return [null, result]
    }, error => {
      return [errorUtils.serialize(error)]
    }).then(responseArgs => {
      event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, ...responseArgs)
    })
  })
}
