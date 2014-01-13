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

# The child_process module also needs to activate the uv loop to make the ipc
# channel setup.
# TODO(zcbenz): Find out why this is needed.
childProcess = require 'child_process'
childProcess.spawn = wrapWithActivateUvLoop childProcess.spawn
childProcess.fork = wrapWithActivateUvLoop childProcess.fork

# Set the __filename to the path of html file if it's file:// protocol.
if window.location.protocol is 'file:'
  global.__filename =
    if process.platform is 'win32'
      window.location.pathname.substr 1
    else
      window.location.pathname
  global.__dirname = path.dirname global.__filename

  # Set module's filename so relative require can work as expected.
  module.filename = global.__filename

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

# Override default window.open.
window.open = (url, name, features) ->
  options = {}
  for feature in features.split ','
    [name, value] = feature.split '='
    options[name] =
      if value is 'yes'
        true
      else if value is 'no'
        false
      else
        value

  options.x ?= options.left
  options.y ?= options.top
  options.title ?= name
  options.width ?= 800
  options.height ?= 600

  BrowserWindow = require('remote').require 'browser-window'
  browser = new BrowserWindow options
  browser.loadUrl url
  browser
