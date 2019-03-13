import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import * as errorUtils from '@electron/internal/common/error-utils'

type IPCHandler = (event: Electron.IpcRendererEvent, ...args: any[]) => any

export const handle = function <T extends IPCHandler> (channel: string, handler: T) {
  ipcRendererInternal.on(channel, (event, requestId, ...args) => {
    new Promise(resolve => resolve(handler(event, ...args))
    ).then(result => {
      return [null, result]
    }, error => {
      return [errorUtils.serialize(error)]
    }).then(responseArgs => {
      event.sender.send(`${channel}_RESPONSE_${requestId}`, ...responseArgs)
    })
  })
}

let nextId = 0

export function invoke<T> (command: string, ...args: any[]) {
  return new Promise<T>((resolve, reject) => {
    const requestId = ++nextId
    ipcRendererInternal.once(`${command}_RESPONSE_${requestId}`, (
      _event, error: Electron.SerializedError, result: any
    ) => {
      if (error) {
        reject(errorUtils.deserialize(error))
      } else {
        resolve(result)
      }
    })
    ipcRendererInternal.send(command, requestId, ...args)
  })
}

export function invokeSync<T> (command: string, ...args: any[]): T {
  const [ error, result ] = ipcRendererInternal.sendSync(command, null, ...args)

  if (error) {
    throw errorUtils.deserialize(error)
  } else {
    return result
  }
}
