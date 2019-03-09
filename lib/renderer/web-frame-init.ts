import { webFrame, WebFrame } from 'electron'
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils'
import * as errorUtils from '@electron/internal/common/error-utils'

// All keys of WebFrame that extend Function
type WebFrameMethod = {
  [K in keyof WebFrame]:
    WebFrame[K] extends Function ? K : never
}

export const webFrameInit = () => {
  // Call webFrame method
  ipcRendererUtils.handle('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', (
    event, method: keyof WebFrameMethod, ...args: any[]
  ) => {
    // The TypeScript compiler cannot handle the sheer number of
    // call signatures here and simply gives up. Incorrect invocations
    // will be caught by "keyof WebFrameMethod" though.
    return (webFrame[method] as any)(...args)
  })

  ipcRendererInternal.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (
    event, requestId: number, method: keyof WebFrameMethod, args: any[]
  ) => {
    new Promise(resolve =>
      // The TypeScript compiler cannot handle the sheer number of
      // call signatures here and simply gives up. Incorrect invocations
      // will be caught by "keyof WebFrameMethod" though.
      (webFrame[method] as any)(...args, resolve)
    ).then(result => {
      return [null, result]
    }, error => {
      return [errorUtils.serialize(error)]
    }).then(responseArgs => {
      event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, ...responseArgs)
    })
  })
}
