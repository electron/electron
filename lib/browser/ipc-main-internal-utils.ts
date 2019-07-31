import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal'
import * as errorUtils from '@electron/internal/common/error-utils'

type IPCHandler = (event: Electron.IpcMainInvokeEvent, ...args: any[]) => any

export const handle = function <T extends IPCHandler> (channel: string, handler: T) {
  ipcMainInternal.handle(channel, handler)
  ipcMainInternal.on(channel, async (event, ...args) => {
    try {
      event.returnValue = [null, await handler(event, ...args)]
    } catch (error) {
      event.returnValue = [errorUtils.serialize(error)]
    }
  })
}

let nextId = 0

export function invokeInWebContents<T> (sender: Electron.WebContentsInternal, sendToAll: boolean, command: string, ...args: any[]) {
  return new Promise<T>((resolve, reject) => {
    const requestId = ++nextId
    const channel = `${command}_RESPONSE_${requestId}`
    ipcMainInternal.on(channel, function handler (
      event, error: Electron.SerializedError, result: any
    ) {
      if (event.sender !== sender) {
        console.error(`Reply to ${command} sent by unexpected WebContents (${event.sender.id})`)
        return
      }

      ipcMainInternal.removeListener(channel, handler)

      if (error) {
        reject(errorUtils.deserialize(error))
      } else {
        resolve(result)
      }
    })

    if (sendToAll) {
      sender._sendInternalToAll(command, requestId, ...args)
    } else {
      sender._sendInternal(command, requestId, ...args)
    }
  })
}
