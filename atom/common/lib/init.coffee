fs     = require 'fs'
path   = require 'path'
timers = require 'timers'
Module = require 'module'

process.atomBinding = (name) ->
  try
    process.binding "atom_#{process.type}_#{name}"
  catch e
    process.binding "atom_common_#{name}" if /No such module/.test e.message

# Add common/api/lib to module search paths.
Module.globalPaths.push path.resolve(__dirname, '..', 'api', 'lib')

# setImmediate and process.nextTick makes use of uv_check and uv_prepare to
# run the callbacks, however since we only run uv loop on requests, the
# callbacks wouldn't be called until something else activated the uv loop,
# which would delay the callbacks for arbitrary long time. So we should
# initiatively activate the uv loop once setImmediate and process.nextTick is
# called.
wrapWithActivateUvLoop = (func) ->
  ->
    process.activateUvLoop()
    func.apply this, arguments
process.nextTick = wrapWithActivateUvLoop process.nextTick
global.setImmediate = wrapWithActivateUvLoop timers.setImmediate
global.clearImmediate = timers.clearImmediate

if process.type is 'browser'
  # setTimeout needs to update the polling timeout of the event loop, when
  # called under Chromium's event loop the node's event loop won't get a chance
  # to update the timeout, so we have to force the node's event loop to
  # recalculate the timeout in browser process.
  global.setTimeout = wrapWithActivateUvLoop timers.setTimeout
  global.setInterval = wrapWithActivateUvLoop timers.setInterval
