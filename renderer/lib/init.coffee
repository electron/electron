path = require 'path'

# Expose information of current process.
process.__atom_type = 'renderer'
process.resourcesPath = path.resolve process.argv[1], '..', '..', '..'

# We modified the original process.argv to let node.js load the
# atom-renderer.js, we need to restore it here.
process.argv.splice 1, 1

# Add renderer/api/lib to require's search paths, which contains javascript part
# of Atom's built-in libraries.
globalPaths = require('module').globalPaths
globalPaths.push path.join process.resourcesPath, 'renderer', 'api', 'lib'

# And also common/api/lib
globalPaths.push path.join process.resourcesPath, 'common', 'api', 'lib'

# Expose global variables.
global.require = require
global.module = module
global.__filename = __filename
global.__dirname = __dirname
