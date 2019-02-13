import * as ipcRenderer from '@electron/internal/renderer/ipc-renderer-internal'
import * as errorUtils from '@electron/internal/common/error-utils'

let nextId = 0

export function invoke (command: string, ...args: any[]) {
  return new Promise((resolve, reject) => {
    const requestId = ++nextId
    ipcRenderer.once(`${command}_RESPONSE_${requestId}`, (
      _event: Electron.Event, error: Electron.SerializedError, result: any
    ) => {
      if (error) {
        reject(errorUtils.deserialize(error))
      } else {
        resolve(result)
      }
    })
    ipcRenderer.send(command, requestId, ...args)
  })
}

export function invokeSync (command: string, ...args: any[]) {
  const [ error, result ] = ipcRenderer.sendSync(command, ...args)

  if (error) {
    throw errorUtils.deserialize(error)
  } else {
    return result
  }
}
