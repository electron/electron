path   = require 'path'
timers = require 'timers'
Module = require 'module'

# Expose information of current process.
process.__atom_type = 'renderer'
process.resourcesPath = path.resolve process.argv[1], '..', '..', '..'

# We modified the original process.argv to let node.js load the
# atom-renderer.js, we need to restore it here.
process.argv.splice 1, 1

# Add renderer/api/lib to require's search paths, which contains javascript part
# of Atom's built-in libraries.
globalPaths = Module.globalPaths
globalPaths.push path.join(process.resourcesPath, 'renderer', 'api', 'lib')
# And also common/api/lib.
globalPaths.push path.join(process.resourcesPath, 'common', 'api', 'lib')
# And also app.
globalPaths.push path.join(process.resourcesPath, 'app')

# Expose global variables.
global.require = require
global.module = module

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

# Set the __filename to the path of html file if it's file:// protocol.
if window.location.protocol is 'file:'
  global.__filename =
    if process.platform is 'win32'
      window.location.pathname.substr 1
    else
      window.location.pathname
  global.__dirname = path.dirname global.__filename

  # Also search for module under the html file.
  module.paths = module.paths.concat Module._nodeModulePaths(global.__dirname)
else
  global.__filename = __filename
  global.__dirname = __dirname

# Redirect window.onerror to uncaughtException.
window.onerror = (error) ->
  if global.process.listeners('uncaughtException').length > 0
    global.process.emit 'uncaughtException', error
    true
  else
    false
