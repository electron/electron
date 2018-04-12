const electron = require('electron')

module.exports = () => {
  // Call webFrame method
  electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', (event, method, args) => {
    electron.webFrame[method](...args)
  })

  electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_SYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
    const result = electron.webFrame[method](...args)
    event.sender.send(`ELECTRON_INTERNAL_BROWSER_SYNC_WEB_FRAME_RESPONSE_${requestId}`, result)
  })

  electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
    const responseCallback = function (result) {
      Promise.resolve(result)
        .then((resolvedResult) => {
          event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, null, resolvedResult)
        })
        .catch((resolvedError) => {
          if (resolvedError instanceof Error) {
            // Errors get lost, because: JSON.stringify(new Error('Message')) === {}
            // Take the serializable properties and construct a generic object
            resolvedError = {
              message: resolvedError.message,
              stack: resolvedError.stack,
              name: resolvedError.name,
              __ELECTRON_SERIALIZED_ERROR__: true
            }
          }

          event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, resolvedError)
        })
    }
    args.push(responseCallback)
    electron.webFrame[method](...args)
  })
}
