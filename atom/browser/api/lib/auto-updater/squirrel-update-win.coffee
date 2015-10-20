ChildProcess = require 'child_process'
fs           = require 'fs'
path         = require 'path'

appFolder             = path.dirname process.execPath # i.e. my-app/app-0.1.13/
rootApplicationFolder = path.resolve appFolder, '..'  # i.e. my-app/
updateDotExe          = path.join rootApplicationFolder, 'Update.exe'
exeName               = path.basename process.execPath

# Spawn a command and invoke the callback when it completes with an error
# and the output from standard out.
spawnUpdate = (args, callback) ->
  stdout = ''

  try
    spawnedProcess = ChildProcess.spawn(updateDotExe, args)
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

processStart = (callback) ->
  spawnUpdate(['--processStart', exeName], callback)

download = (callback) ->
  spawnUpdate ['--download', @updateUrl], (error, stdout) ->
    return callback(error) if error?

    try
      # Last line of output is the JSON details about the releases
      json   = stdout.trim().split('\n').pop()
      update = JSON.parse(json)?.releasesToApply?.pop?()
    catch error
      error.stdout = stdout
      return callback(error)

    callback(null, update)

update = (updateUrl, callback) ->
  spawnUpdate ['--update', updateUrl], callback

# Is the Update.exe installed with the current application?
exports.supported = ->
  fs.accessSync(updateDotExe, fs.R_OK)
exports.processStart = processStart
exports.download     = download
exports.update       = update
