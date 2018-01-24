const fs = require('fs')
const path = require('path')
const spawn = require('child_process').spawn

// i.e. my-app/app-0.1.13/
const appFolder = path.dirname(process.execPath)

// i.e. my-app/Update.exe
const updateExe = path.resolve(appFolder, '..', 'Update.exe')
const exeName = path.basename(process.execPath)
let spawnedArgs = []
let spawnedProcess

const isSameArgs = (args) => args.length === spawnedArgs.length && args.every((e, i) => e === spawnedArgs[i])

// Spawn a command and invoke the callback when it completes with an error
// and the output from standard out.
let spawnUpdate = function (args, detached, callback) {
  try {
    // Ensure we don't spawn multiple squirrel processes
    // Process spawned, same args:        Attach events to alread running process
    // Process spawned, different args:   Return with error
    // No process spawned:                Spawn new process
    if (spawnedProcess && !isSameArgs(args)) {
      // Disabled for backwards compatibility:
      // eslint-disable-next-line standard/no-callback-literal
      return callback(`AutoUpdater process with arguments ${args} is already running`)
    } else if (!spawnedProcess) {
      spawnedProcess = spawn(updateExe, args, {
        detached: detached,
        windowsHide: true
      })
      spawnedArgs = args || []
    }
  } catch (error) {
    // Shouldn't happen, but still guard it.
    process.nextTick(() => callback(error))
    return
  }

  let stdout, stderr
  spawnedProcess.stdout.on('data', (data) => { stdout += data })
  spawnedProcess.stderr.on('data', (data) => { stderr += data })

  let errorEmitted = false
  spawnedProcess.on('error', (error) => {
    errorEmitted = true
    callback(error)
  })

  return spawnedProcess.on('exit', (code, signal) => {
    spawnedProcess = undefined
    spawnedArgs = []

    if (errorEmitted) return
    // Process terminated with error.
    if (code !== 0) {
      // Disabled for backwards compatibility:
      // eslint-disable-next-line standard/no-callback-literal
      return callback(`Command failed: ${signal != null ? signal : code}\n${stderr}`)
    }
    callback(null, stdout)
  })
}

// Start an instance of the installed app.
exports.processStart = () => {
  return spawnUpdate(['--processStartAndWait', exeName], true, () => {})
}

// Download the releases specified by the URL and write new results to stdout.
exports.checkForUpdate = (updateURL, callback) => {
  return spawnUpdate(['--checkForUpdate', updateURL], false, (error, stdout) => {
    if (error != null) return callback(error)

    let update
    try {
      const json = JSON.parse(stdout.trim().split('\n').pop())
      if (json.releasesToApply !== null) {
        if (typeof json.releasesToApply.pop === 'function') {
          update = json.releasesToApply.pop()
        }
      }
    } catch (jsonError) {
      // Disabled for backwards compatibility:
      // eslint-disable-next-line standard/no-callback-literal
      return callback(`Invalid result:\n${stdout}`)
    }
    return callback(null, update)
  })
}

// Update the application to the latest remote version specified by URL.
exports.update = (updateURL, callback) => {
  return spawnUpdate(['--update', updateURL], false, callback)
}

// Is the Update.exe installed with the current application?
exports.supported = () => {
  try {
    fs.accessSync(updateExe, fs.R_OK)
    return true
  } catch (error) {
    return false
  }
}
