'use strict'

const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal')
const errorUtils = require('@electron/internal/common/error-utils')

const callHandler = async function (handler, event, args, reply) {
  try {
    const result = await handler(event, ...args)
    reply([null, result])
  } catch (error) {
    reply([errorUtils.serialize(error)])
  }
}

exports.handle = function (channel, handler) {
  ipcMainInternal.on(channel, (event, requestId, ...args) => {
    callHandler(handler, event, args, responseArgs => {
      event._replyInternal(`${channel}_RESPONSE_${requestId}`, ...responseArgs)
    })
  })
}

exports.handleSync = function (channel, handler) {
  ipcMainInternal.on(channel, (event, ...args) => {
    callHandler(handler, event, args, responseArgs => {
      event.returnValue = responseArgs
    })
  })
}
