fs = require 'fs'
path = require 'path'

# Expose information of current process.
process.__atom_type = 'browser'
process.resourcesPath = path.resolve process.argv[1], '..', '..', '..'

# We modified the original process.argv to let node.js load the atom.js,
# we need to restore it here.
process.argv.splice 1, 1

if process.platform is 'win32'
  # Redirect node's console to use our own implementations, since node can not
  # handle output when running as GUI program.
  console.log = console.error = console.warn = process.log
  process.stdout.write = process.stderr.write = process.log

  # Always returns EOF for stdin stream.
  Readable = require('stream').Readable
  stdin = new Readable
  stdin.push null
  process.__defineGetter__ 'stdin', -> stdin

# Add browser/api/lib to require's search paths,
# which contains javascript part of Atom's built-in libraries.
globalPaths = require('module').globalPaths
globalPaths.push path.join process.resourcesPath, 'browser', 'api', 'lib'

# And also common/api/lib
globalPaths.push path.join process.resourcesPath, 'common', 'api', 'lib'

# Do loading in next tick since we still need some initialize work before
# native bindings can work.
setImmediate ->
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

  packagePath = path.join process.resourcesPath, 'app'
  try
    # First we try to load process.resourcesPath/app
    packageJson = JSON.parse(fs.readFileSync(path.join(packagePath, 'package.json')))
  catch error
    # If not found then we load browser/default_app
    packagePath = path.join process.resourcesPath, 'browser', 'default_app'
    packageJson = JSON.parse(fs.readFileSync(path.join(packagePath, 'package.json')))

  # Set application's version.
  app = require 'app'
  app.setVersion packageJson.version if packageJson.version?

  # Set application's name.
  if packageJson.productName?
    app.setName packageJson.productName
  else if packageJson.name?
    app.setName packageJson.name

  # Finally load app's main.js and transfer control to C++.
  require path.join(packagePath, packageJson.main)
