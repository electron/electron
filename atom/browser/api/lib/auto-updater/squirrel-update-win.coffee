ChildProcess = require 'child_process'
fs           = require 'fs'
path         = require 'path'

appFolder             = path.resolve(process.execPath, '..') # i.e. my-app/app-0.1.13/
rootApplicationFolder = path.resolve(appFolder, '..') # i.e. my-app/
updateDotExe          = path.join(rootApplicationFolder, 'Update.exe')
exeName               = path.basename(process.execPath)

# Spawn a command and invoke the callback when it completes with an error
# and the output from standard out.
spawn = (command, args, callback) ->
  stdout = ''

  try
    spawnedProcess = ChildProcess.spawn(command, args)
  catch error
    # Spawn can throw an error
    process.nextTick -> callback?(error, stdout)
    return

  spawnedProcess.stdout.on 'data', (data) -> stdout += data

  error = null
  spawnedProcess.on 'error', (processError) -> error ?= processError
  spawnedProcess.on 'close', (code, signal) ->
    error ?= new Error("Command failed: #{signal ? code}") if code isnt 0
    error?.code ?= code
    error?.stdout ?= stdout
    callback?(error, stdout)

# Spawn the Update.exe with the given arguments and invoke the callback when
# the command completes.
spawnUpdate = (args, callback) ->
  spawn(updateDotExe, args, callback)

# Exports
exports.spawn = spawnUpdate

# Is the Update.exe installed with Atom?
exports.existsSync = ->
  fs.accessSync(updateDotExe, fs.R_OK)
