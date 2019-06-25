'use strict'

const timers = require('timers')
const util = require('util')

process.atomBinding = require('@electron/internal/common/atom-binding-setup')(process._linkedBinding, process.type)

// setImmediate and process.nextTick makes use of uv_check and uv_prepare to
// run the callbacks, however since we only run uv loop on requests, the
// callbacks wouldn't be called until something else activated the uv loop,
// which would delay the callbacks for arbitrary long time. So we should
// initiatively activate the uv loop once setImmediate and process.nextTick is
// called.
const wrapWithActivateUvLoop = function (func) {
  return wrap(func, function (func) {
    return function () {
      process.activateUvLoop()
      return func.apply(this, arguments)
    }
  })
}

function wrap (func, wrapper) {
  const wrapped = wrapper(func)
  if (func[util.promisify.custom]) {
    wrapped[util.promisify.custom] = wrapper(func[util.promisify.custom])
  }
  return wrapped
}

process.nextTick = wrapWithActivateUvLoop(process.nextTick)

global.setImmediate = timers.setImmediate = wrapWithActivateUvLoop(timers.setImmediate)
global.clearImmediate = timers.clearImmediate

// setTimeout needs to update the polling timeout of the event loop, when
// called under Chromium's event loop the node's event loop won't get a chance
// to update the timeout, so we have to force the node's event loop to
// recalculate the timeout in browser process.
timers.setTimeout = wrapWithActivateUvLoop(timers.setTimeout)
timers.setInterval = wrapWithActivateUvLoop(timers.setInterval)

// Only override the global setTimeout/setInterval impls in the browser process
if (process.type === 'browser') {
  global.setTimeout = timers.setTimeout
  global.setInterval = timers.setInterval
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
