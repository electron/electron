fs = require 'fs'
path = require 'path'

# Provide default Content API implementations.
atom = {}

atom.browserMainParts =
  preMainMessageLoopRun: ->
    # This is the start of the whole application, usually we should initialize
    # the main window here.

# Store atom object in global scope, apps can just override methods of it to
# implement various logics.
global.__atom = atom

# Add Atom.app/Contents/Resources/browser/api/lib to require's search paths,
# which contains javascript of Atom's built-in libraries.  
require('module').globalPaths.push path.join(__dirname, '..', 'api', 'lib')

# Don't quit on fatal error.
process.on 'uncaughtException', (error) ->
  # TODO Show error in GUI.
  message = error.stack ? "#{error.name}: #{error.message}"
  console.error 'uncaughtException:'
  console.error message

# Now we try to load app's package.json.
packageJson = null

packagePath = path.join __dirname, '..', '..', 'app'
try
  # First we try to load Atom.app/Contents/Resources/app
  packageJson = JSON.parse(fs.readFileSync(path.join(packagePath, 'package.json')))
catch error
  # If not found then we load Atom.app/Contents/Resources/browser/default_app
  packagePath = path.join __dirname, '..', 'default_app'
  packageJson = JSON.parse(fs.readFileSync(path.join(packagePath, 'package.json')))

# Finally load app's main.js and transfer control to C++.
require path.join(packagePath, packageJson.main)
