fs      = require 'fs'
path    = require 'path'
{spawn} = require 'child_process'

appFolder = path.dirname process.execPath # i.e. my-app/app-0.1.13/
updateExe = path.resolve appFolder, '..', 'Update.exe' # i.e. my-app/Update.exe
exeName   = path.basename process.execPath

# Spawn a command and invoke the callback when it completes with an error
# and the output from standard out.
spawnUpdate = (args, detached, callback) ->
  try
    spawnedProcess = spawn updateExe, args, {detached}
  catch error
    # Shouldn't happen, but still guard it.
    process.nextTick -> callback error
    return

  stdout = ''
  stderr = ''
  spawnedProcess.stdout.on 'data', (data) -> stdout += data
  spawnedProcess.stderr.on 'data', (data) -> stderr += data

  errorEmitted = false
  spawnedProcess.on 'error', (error) ->
    errorEmitted = true
    callback error
  spawnedProcess.on 'exit', (code, signal) ->
    # We may have already emitted an error.
    return if errorEmitted

    # Process terminated with error.
    if code isnt 0
      return callback "Command failed: #{signal ? code}\n#{stderr}"

    # Success.
    callback null, stdout

# Start an instance of the installed app.
exports.processStart = (callback) ->
  spawnUpdate ['--processStart', exeName], true, ->

# Download the releases specified by the URL and write new results to stdout.
exports.download = (updateUrl, callback) ->
  spawnUpdate ['--download', updateUrl], false, (error, stdout) ->
    return callback(error) if error?

    try
      # Last line of output is the JSON details about the releases
      json = stdout.trim().split('\n').pop()
      update = JSON.parse(json)?.releasesToApply?.pop?()
    catch
      return callback "Invalid result:\n#{stdout}"

    callback null, update

# Update the application to the latest remote version specified by URL.
exports.update = (updateUrl, callback) ->
  spawnUpdate ['--update', updateUrl], false, callback

# Is the Update.exe installed with the current application?
exports.supported = ->
  try
    fs.accessSync updateExe, fs.R_OK
    return true
  catch
    return false
