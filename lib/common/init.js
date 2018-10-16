'use strict'

const timers = require('timers')
const asyncHooks = require('async_hooks')

process.atomBinding = require('@electron/internal/common/atom-binding-setup')(process.binding, process.type)

// setImmediate and process.nextTick makes use of uv_check and uv_prepare to
// run the callbacks, however since we only run uv loop on requests, the
// callbacks wouldn't be called until something else activated the uv loop,
// which would delay the callbacks for arbitrary long time. So we should
// initiatively activate the uv loop once setImmediate and process.nextTick is
// called.
const asyncHook = asyncHooks.createHook({
  init (asyncId, type, triggerAsyncId) {
    switch (type.toUpperCase()) {
      case 'IMMEDIATE':
      case 'TIMEOUT':
      case 'TICKOBJECT':
        process.activateUvLoop()
        break
        
      default:
        break
    }
  }

})
asyncHook.enable()

if (process.type === 'renderer') {
  global.setImmediate = timers.setImmediate
  global.clearImmediate = timers.clearImmediate
}

if (process.platform === 'win32') {
  // Always returns EOF for stdin stream.
  const { Readable } = require('stream')
  const stdin = new Readable()
  stdin.push(null)
  process.__defineGetter__('stdin', function () {
    return stdin
  })
}
