path = require 'path'
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

# Set the __filename to the path of html file if it's file:// protocol.
if window.location.protocol is 'file:'
  global.__filename = window.location.pathname
  global.__dirname = path.dirname global.__filename

  # Also search for module under the html file.
  module.paths = module.paths.concat Module._nodeModulePaths(global.__dirname)
else
  global.__filename = __filename
  global.__dirname = __dirname
