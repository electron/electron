const timers = require('timers')

process.atomBinding = function (name) {
  try {
    return process.binding('atom_' + process.type + '_' + name)
  } catch (error) {
    if (/No such module/.test(error.message)) {
      return process.binding('atom_common_' + name)
    } else {
      throw error
    }
  }
}

// setImmediate and process.nextTick makes use of uv_check and uv_prepare to
// run the callbacks, however since we only run uv loop on requests, the
// callbacks wouldn't be called until something else activated the uv loop,
// which would delay the callbacks for arbitrary long time. So we should
// initiatively activate the uv loop once setImmediate and process.nextTick is
// called.
var wrapWithActivateUvLoop = function (func) {
  return function () {
    process.activateUvLoop()
    return func.apply(this, arguments)
  }
}

process.nextTick = wrapWithActivateUvLoop(process.nextTick)

global.setImmediate = wrapWithActivateUvLoop(timers.setImmediate)

global.clearImmediate = timers.clearImmediate

if (process.type === 'browser') {
  // setTimeout needs to update the polling timeout of the event loop, when
  // called under Chromium's event loop the node's event loop won't get a chance
  // to update the timeout, so we have to force the node's event loop to
  // recalculate the timeout in browser process.
  global.setTimeout = wrapWithActivateUvLoop(timers.setTimeout)
  global.setInterval = wrapWithActivateUvLoop(timers.setInterval)
}

if (process.platform === 'win32') {
  // Always returns EOF for stdin stream.
  const {Readable} = require('stream')
  const stdin = new Readable()
  stdin.push(null)
  process.__defineGetter__('stdin', function () {
    return stdin
  })

  // If we're running as a Windows Store app, __dirname will be set
  // to C:/Program Files/WindowsApps.
  //
  // Nobody else get's to install there, changing the path is forbidden
  // We can therefore say that we're running as appx
  if (__dirname.indexOf('\\Program Files\\WindowsApps\\') === 2) {
    process.windowsStore = true
  }
}
