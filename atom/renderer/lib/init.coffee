process = global.process
events  = require 'events'
path    = require 'path'
url     = require 'url'
Module  = require 'module'

# We modified the original process.argv to let node.js load the
# atom-renderer.js, we need to restore it here.
process.argv.splice 1, 1

# Add renderer/api/lib to require's search paths, which contains javascript part
# of Atom's built-in libraries.
globalPaths = Module.globalPaths
globalPaths.push path.resolve(__dirname, '..', 'api', 'lib')
# And also app.
globalPaths.push path.join(process.resourcesPath, 'app')
globalPaths.push path.join(process.resourcesPath, 'app.asar')

# Import common settings.
require path.resolve(__dirname, '..', '..', 'common', 'lib', 'init')

# The global variable will be used by ipc for event dispatching
v8Util = process.atomBinding 'v8_util'
v8Util.setHiddenValue global, 'ipc', new events.EventEmitter

# Process command line arguments.
nodeIntegration = 'false'
for arg in process.argv
  if arg.indexOf('--guest-instance-id=') == 0
    # This is a guest web view.
    process.guestInstanceId = parseInt arg.substr(arg.indexOf('=') + 1)
    # Set the frame name to make AtomRendererClient recognize this guest.
    require('web-frame').setName 'ATOM_SHELL_GUEST_WEB_VIEW'
  else if arg.indexOf('--node-integration=') == 0
    nodeIntegration = arg.substr arg.indexOf('=') + 1
  else if arg.indexOf('--preload=') == 0
    preloadScript = arg.substr arg.indexOf('=') + 1

if location.protocol is 'chrome-devtools:'
  # Override some inspector APIs.
  require './inspector'
  nodeIntegration = 'true'
else if location.protocol is 'chrome-extension:'
  # Add implementations of chrome API.
  require './chrome-api'
  nodeIntegration = 'true'
else
  # Override default web functions.
  require './override'
  # Load webview tag implementation.
  unless process.guestInstanceId?
    require './web-view/web-view'
    require './web-view/web-view-attributes'

if nodeIntegration in ['true', 'all', 'except-iframe', 'manual-enable-iframe']
  # Export node bindings to global.
  global.require = require
  global.module = module

  # Set the __filename to the path of html file if it is file: protocol.
  if window.location.protocol is 'file:'
    pathname =
      if process.platform is 'win32' and window.location.pathname[0] is '/'
        window.location.pathname.substr 1
      else
        window.location.pathname
    global.__filename = path.normalize decodeURIComponent(pathname)
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

  # Emit the 'exit' event when page is unloading.
  window.addEventListener 'unload', ->
    process.emit 'exit'
else
  # The Module.runMain will run process._tickCallck() immediately, so we are
  # able to delete the symbols in this tick even though we used process.nextTick
  # to schedule it.
  # It is important that we put this in process.nextTick, if we delete them now
  # some code in node.js will complain about "process not defined".
  process.nextTick ->
    delete global.process
    delete global.setImmediate
    delete global.clearImmediate

# Load the script specfied by the "preload" attribute.
if preloadScript
  try
    require preloadScript
  catch error
    throw error unless error.code is 'MODULE_NOT_FOUND'
    console.error "Unable to load preload script #{preloadScript}"
