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

const isSameArgs = (args) => {
  return (args.length === spawnedArgs.length) && args.every((e, i) => {
    return e === spawnedArgs[i]
  })
}

// Spawn a command and invoke the callback when it completes with an error
// and the output from standard out.
function spawnUpdate (args, detached, callback) {
  try {
    // Ensure we don't spawn multiple squirrel processes
    // Process spawned, same args:        Attach events to alread running process
    // Process spawned, different args:   Return with error
    // No process spawned:                Spawn new process
    if (spawnedProcess && !isSameArgs(args)) {
      return callback(`AutoUpdater process with arguments ${args} is already running`)
    } else if (!spawnedProcess) {
      spawnedProcess = spawn(updateExe, args, {
        detached: detached
      })
      spawnedArgs = args || []
    }
  } catch (error) {
    process.nextTick(() => callback(error))
    return
  }

  let stdout = ''
  let stderr = ''
  let errorEmitted = false

  spawnedProcess.stdout.on('data', (data) => { stdout += data })
  spawnedProcess.stderr.on('data', (data) => { stderr += data })

  spawnedProcess.on('error', (error) => {
    errorEmitted = true
    callback(error)
  })

  return spawnedProcess.on('exit', (code, signal) => {
    spawnedProcess = undefined
    spawnedArgs = []

    if (errorEmitted) return

    if (code !== 0) {
      const returnSignal = signal != null ? signal : code
      return callback(`Command failed: ${returnSignal}\n${stderr}`)
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
      const jsonFunction = (typeof json.pop === 'function') ? json.pop() : undefined
      const releases = (json.releasesToApply != null) ? jsonFunction : undefined
      update = (json != null) ? releases : undefined
    } catch (error) {
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
