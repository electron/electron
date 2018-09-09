const {ipcRenderer, webFrame} = require('electron')
const errorUtils = require('../common/error-utils')

module.exports = () => {
  // Call webFrame method
  ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', (event, method, args) => {
    webFrame[method](...args)
  })

  ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_SYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
    const result = webFrame[method](...args)
    event.sender.send(`ELECTRON_INTERNAL_BROWSER_SYNC_WEB_FRAME_RESPONSE_${requestId}`, result)
  })

  ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
    const responseCallback = function (result) {
      Promise.resolve(result)
        .then((resolvedResult) => {
          event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, null, resolvedResult)
        })
        .catch((resolvedError) => {
          event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, errorUtils.serialize(resolvedError))
        })
    }
    args.push(responseCallback)
    webFrame[method](...args)
  })
}
