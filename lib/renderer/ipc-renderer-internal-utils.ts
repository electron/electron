import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import * as errorUtils from '@electron/internal/common/error-utils'

let nextId = 0

export function invoke<T> (command: string, ...args: any[]) {
  return new Promise<T>((resolve, reject) => {
    const requestId = ++nextId
    ipcRendererInternal.once(`${command}_RESPONSE_${requestId}`, (
      _event: Electron.Event, error: Electron.SerializedError, result: any
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
  const [ error, result ] = ipcRendererInternal.sendSync(command, ...args)

  if (error) {
    throw errorUtils.deserialize(error)
  } else {
    return result
  }
}
