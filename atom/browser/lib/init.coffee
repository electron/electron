fs     = require 'fs'
path   = require 'path'
util   = require 'util'
Module = require 'module'

# We modified the original process.argv to let node.js load the atom.js,
# we need to restore it here.
process.argv.splice 1, 1

# Clear search paths.
require path.resolve(__dirname, '..', '..', 'common', 'lib', 'reset-search-paths')

# Import common settings.
require path.resolve(__dirname, '..', '..', 'common', 'lib', 'init')

# Add browser/api/lib to module search paths, which contains javascript part of
# Electron's built-in libraries.
globalPaths = Module.globalPaths
globalPaths.push path.resolve(__dirname, '..', 'api', 'lib')

# Don't quit on fatal error.
process.on 'uncaughtException', (error) ->
  # Do nothing if the user has a custom uncaught exception handler.
  if process.listeners('uncaughtException').length > 1
    return

  # Show error in GUI.
  stack = error.stack ? "#{error.name}: #{error.message}"
  message = "Uncaught Exception:\n#{stack}"
  require('dialog').showErrorBox 'A JavaScript error occurred in the main process', message

# Emit 'exit' event on quit.
app = require 'app'
app.on 'quit', ->
  process.emit 'exit'

# Load the RPC server.
require './rpc-server'

# Load the guest view manager.
require './guest-view-manager'
require './guest-window-manager'

# Now we try to load app's package.json.
packageJson = null

searchPaths = [ 'app', 'app.asar', 'default_app' ]
for packagePath in searchPaths
  try
    packagePath = path.join process.resourcesPath, packagePath
    packageJson = JSON.parse(fs.readFileSync(path.join(packagePath, 'package.json')))
    break
  catch e
    continue

unless packageJson?
  process.nextTick -> process.exit 1
  throw new Error("Unable to find a valid app")

# Set application's version.
app.setVersion packageJson.version if packageJson.version?

# Set application's name.
if packageJson.productName?
  app.setName packageJson.productName
else if packageJson.name?
  app.setName packageJson.name

# Set application's desktop name.
if packageJson.desktopName?
  app.setDesktopName packageJson.desktopName
else
  app.setDesktopName "#{app.getName()}.desktop"

# Chrome 42 disables NPAPI plugins by default, reenable them here
app.commandLine.appendSwitch 'enable-npapi'

# Set the user path according to application's name.
app.setPath 'userData', path.join(app.getPath('appData'), app.getName())
app.setPath 'userCache', path.join(app.getPath('cache'), app.getName())
app.setAppPath packagePath

# Load the chrome extension support.
require './chrome-extension'

# Set main startup script of the app.
mainStartupScript = packageJson.main or 'index.js'

# Finally load app's main.js and transfer control to C++.
Module._load path.join(packagePath, mainStartupScript), Module, true
