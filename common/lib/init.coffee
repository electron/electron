path   = require 'path'
timers = require 'timers'
Module = require 'module'

# Add common/api/lib to module search paths.
globalPaths = Module.globalPaths
globalPaths.push path.join(process.resourcesPath, 'common', 'api', 'lib')

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

# The child_process module also needs to activate the uv loop to make the ipc
# channel setup.
# TODO(zcbenz): Find out why this is needed.
childProcess = require 'child_process'
childProcess.spawn = wrapWithActivateUvLoop childProcess.spawn
childProcess.fork = wrapWithActivateUvLoop childProcess.fork
