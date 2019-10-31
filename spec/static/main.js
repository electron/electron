// Deprecated APIs are still supported and should be tested.
process.throwDeprecation = false

const electron = require('electron')
const { app, BrowserWindow, crashReporter, dialog, ipcMain, protocol, webContents } = electron

const fs = require('fs')
const path = require('path')
const util = require('util')
const v8 = require('v8')

const argv = require('yargs')
  .boolean('ci')
  .string('g').alias('g', 'grep')
  .boolean('i').alias('i', 'invert')
  .argv

let window = null

// will be used by crash-reporter spec.
process.port = 0

v8.setFlagsFromString('--expose_gc')
app.commandLine.appendSwitch('js-flags', '--expose_gc')
app.commandLine.appendSwitch('ignore-certificate-errors')
app.commandLine.appendSwitch('disable-renderer-backgrounding')

// Disable security warnings (the security warnings test will enable them)
process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = true

// Accessing stdout in the main process will result in the process.stdout
// throwing UnknownSystemError in renderer process sometimes. This line makes
// sure we can reproduce it in renderer process.
// eslint-disable-next-line
process.stdout

// Access console to reproduce #3482.
// eslint-disable-next-line
console

ipcMain.on('message', function (event, ...args) {
  event.sender.send('message', ...args)
})

ipcMain.handle('get-temp-dir', () => app.getPath('temp'))
ipcMain.handle('ping', () => null)

// Set productName so getUploadedReports() uses the right directory in specs
if (process.platform !== 'darwin') {
  crashReporter.productName = 'Zombies'
}

// Write output to file if OUTPUT_TO_FILE is defined.
const outputToFile = process.env.OUTPUT_TO_FILE
const print = function (_, method, args) {
  const output = util.format.apply(null, args)
  if (outputToFile) {
    fs.appendFileSync(outputToFile, output + '\n')
  } else {
    console[method](output)
  }
}
ipcMain.on('console-call', print)

ipcMain.on('process.exit', function (event, code) {
  process.exit(code)
})

ipcMain.on('eval', function (event, script) {
  event.returnValue = eval(script) // eslint-disable-line
})

ipcMain.on('echo', function (event, msg) {
  event.returnValue = msg
})

global.setTimeoutPromisified = util.promisify(setTimeout)

process.removeAllListeners('uncaughtException')
process.on('uncaughtException', function (error) {
  console.error(error, error.stack)
  process.exit(1)
})

global.nativeModulesEnabled = !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS

// Register app as standard scheme.
global.standardScheme = 'app'
global.zoomScheme = 'zoom'
protocol.registerSchemesAsPrivileged([
  { scheme: global.standardScheme, privileges: { standard: true, secure: true } },
  { scheme: global.zoomScheme, privileges: { standard: true, secure: true } },
  { scheme: 'cors', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'cors-blob', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'no-cors', privileges: { supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } }
])

app.on('window-all-closed', function () {
  app.quit()
})

app.on('gpu-process-crashed', (event, killed) => {
  console.log(`GPU process crashed (killed=${killed})`)
})

app.on('renderer-process-crashed', (event, contents, killed) => {
  console.log(`webContents ${contents.id} crashed: ${contents.getURL()} (killed=${killed})`)
})

app.on('ready', function () {
  // Test if using protocol module would crash.
  electron.protocol.registerStringProtocol('test-if-crashes', function () {})

  // Send auto updater errors to window to be verified in specs
  electron.autoUpdater.on('error', function (error) {
    window.send('auto-updater-error', error.message)
  })

  window = new BrowserWindow({
    title: 'Electron Tests',
    show: false,
    width: 800,
    height: 600,
    webPreferences: {
      backgroundThrottling: false,
      nodeIntegration: true,
      webviewTag: true
    }
  })
  window.loadFile('static/index.html', {
    query: {
      grep: argv.grep,
      invert: argv.invert ? 'true' : ''
    }
  })
  window.on('unresponsive', function () {
    const chosen = dialog.showMessageBox(window, {
      type: 'warning',
      buttons: ['Close', 'Keep Waiting'],
      message: 'Window is not responsing',
      detail: 'The window is not responding. Would you like to force close it or just keep waiting?'
    })
    if (chosen === 0) window.destroy()
  })
  window.webContents.on('crashed', function () {
    console.error('Renderer process crashed')
    process.exit(1)
  })
})

for (const eventName of [
  'remote-get-guest-web-contents'
]) {
  ipcMain.on(`handle-next-${eventName}`, function (event, returnValue) {
    event.sender.once(eventName, (event) => {
      if (returnValue) {
        event.returnValue = returnValue
      } else {
        event.preventDefault()
      }
    })
  })
}

ipcMain.on('set-client-certificate-option', function (event, skip) {
  app.once('select-client-certificate', function (event, webContents, url, list, callback) {
    event.preventDefault()
    if (skip) {
      callback()
    } else {
      ipcMain.on('client-certificate-response', function (event, certificate) {
        callback(certificate)
      })
      window.webContents.send('select-client-certificate', webContents.id, list)
    }
  })
  event.returnValue = 'done'
})

ipcMain.on('prevent-next-will-attach-webview', (event) => {
  event.sender.once('will-attach-webview', event => event.preventDefault())
})

ipcMain.on('disable-node-on-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.src = `file://${path.join(__dirname, '..', 'fixtures', 'pages', 'c.html')}`
    webPreferences.nodeIntegration = false
  })
})

ipcMain.on('disable-preload-on-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.src = `file://${path.join(__dirname, '..', 'fixtures', 'pages', 'webview-stripped-preload.html')}`
    delete webPreferences.preload
    delete webPreferences.preloadURL
  })
})

ipcMain.on('handle-uncaught-exception', (event, message) => {
  suspendListeners(process, 'uncaughtException', (error) => {
    event.returnValue = error.message
  })
  fs.readFile(__filename, () => {
    throw new Error(message)
  })
})

ipcMain.on('handle-unhandled-rejection', (event, message) => {
  suspendListeners(process, 'unhandledRejection', (error) => {
    event.returnValue = error.message
  })
  fs.readFile(__filename, () => {
    Promise.reject(new Error(message))
  })
})

// Suspend listeners until the next event and then restore them
const suspendListeners = (emitter, eventName, callback) => {
  const listeners = emitter.listeners(eventName)
  emitter.removeAllListeners(eventName)
  emitter.once(eventName, (...args) => {
    emitter.removeAllListeners(eventName)
    listeners.forEach((listener) => {
      emitter.on(eventName, listener)
    })

    // eslint-disable-next-line standard/no-callback-literal
    callback(...args)
  })
}
