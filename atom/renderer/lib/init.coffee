path   = require 'path'
url    = require 'url'
Module = require 'module'

# Expose information of current process.
process.type = 'renderer'
process.resourcesPath = path.resolve process.argv[1], '..', '..', '..', '..'

# We modified the original process.argv to let node.js load the
# atom-renderer.js, we need to restore it here.
process.argv.splice 1, 1

# Add renderer/api/lib to require's search paths, which contains javascript part
# of Atom's built-in libraries.
globalPaths = Module.globalPaths
globalPaths.push path.join(process.resourcesPath, 'atom', 'renderer', 'api', 'lib')
# And also app.
globalPaths.push path.join(process.resourcesPath, 'app')

# Import common settings.
require path.resolve(__dirname, '..', '..', 'common', 'lib', 'init.js')

# Expose global variables.
global.require = require
global.module = module

# Emit the 'exit' event when page is unloading.
window.addEventListener 'unload', ->
  process.emit 'exit'

# Set the __filename to the path of html file if it's file: or asar: protocol.
if window.location.protocol in ['file:', 'asar:']
  pathname =
    if process.platform is 'win32' and window.location.pathname[0] is '/'
      window.location.pathname.substr 1
    else
      window.location.pathname
  global.__filename = decodeURIComponent pathname
  global.__dirname = path.dirname global.__filename

  # Set module's filename so relative require can work as expected.
  module.filename = global.__filename

  # Also search for module under the html file.
  module.paths = module.paths.concat Module._nodeModulePaths(global.__dirname)
else
  global.__filename = __filename
  global.__dirname = __dirname

if location.protocol is 'chrome-devtools:'
  # Override some inspector APIs.
  require path.join(__dirname, 'inspector')
else if location.protocol is 'chrome-extension:'
  # Add implementations of chrome API.
  require path.join(__dirname, 'chrome-api')
else
  # Override default web functions.
  require path.join(__dirname, 'override')
