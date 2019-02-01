'use strict'

const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')
const errorUtils = require('@electron/internal/common/error-utils')

let nextId = 0

exports.invoke = function (command, ...args) {
  return new Promise((resolve, reject) => {
    const requestId = ++nextId
    ipcRenderer.once(`${command}_RESPONSE_${requestId}`, (event, error, result) => {
      if (error) {
        reject(errorUtils.deserialize(error))
      } else {
        resolve(result)
      }
    })
    ipcRenderer.send(command, requestId, ...args)
  })
}

exports.invokeSync = function (command, ...args) {
  const [ error, result ] = ipcRenderer.sendSync(command, ...args)

  if (error) {
    throw errorUtils.deserialize(error)
  } else {
    return result
  }
}
