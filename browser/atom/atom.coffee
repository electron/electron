fs = require 'fs'
path = require 'path'

# Enable idle gc.
process.atomBinding('idle_gc').start()

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
# which contains javascript part of Atom's built-in libraries.  
globalPaths = require('module').globalPaths
globalPaths.push path.join(__dirname, '..', 'api', 'lib')

# And Atom.app/Contents/Resources/common/api/lib
globalPaths.push path.join(__dirname, '..', '..', 'common', 'api', 'lib')

# Don't quit on fatal error.
process.on 'uncaughtException', (error) ->
  # Show error in GUI.
  message = error.stack ? "#{error.name}: #{error.message}"
  require('dialog').showMessageBox
      type: 'warning'
      title: 'An javascript error occured in the browser'
      message: 'uncaughtException'
      detail: message
      buttons: ['OK']

# Load the RPC server.
require './rpc-server.js'

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
