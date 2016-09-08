// Disable use of deprecated functions.
process.throwDeprecation = true

const electron = require('electron')
const app = electron.app
const crashReporter = electron.crashReporter
const ipcMain = electron.ipcMain
const dialog = electron.dialog
const BrowserWindow = electron.BrowserWindow
const protocol = electron.protocol

const Coverage = require('electabul').Coverage
const fs = require('fs')
const path = require('path')
const url = require('url')
const util = require('util')

var argv = require('yargs')
  .boolean('ci')
  .string('g').alias('g', 'grep')
  .boolean('i').alias('i', 'invert')
  .argv

var window = null
process.port = 0 // will be used by crash-reporter spec.

app.commandLine.appendSwitch('js-flags', '--expose_gc')
app.commandLine.appendSwitch('ignore-certificate-errors')
app.commandLine.appendSwitch('disable-renderer-backgrounding')

// Accessing stdout in the main process will result in the process.stdout
// throwing UnknownSystemError in renderer process sometimes. This line makes
// sure we can reproduce it in renderer process.
process.stdout

// Access console to reproduce #3482.
console

ipcMain.on('message', function (event, ...args) {
  event.sender.send('message', ...args)
})

// Set productName so getUploadedReports() uses the right directory in specs
if (process.platform === 'win32') {
  crashReporter.productName = 'Zombies'
}

// Write output to file if OUTPUT_TO_FILE is defined.
const outputToFile = process.env.OUTPUT_TO_FILE
const print = function (_, args) {
  let output = util.format.apply(null, args)
  if (outputToFile) {
    fs.appendFileSync(outputToFile, output + '\n')
  } else {
    console.error(output)
  }
}
ipcMain.on('console.log', print)
ipcMain.on('console.error', print)

ipcMain.on('process.exit', function (event, code) {
  process.exit(code)
})

ipcMain.on('eval', function (event, script) {
  event.returnValue = eval(script) // eslint-disable-line
})

ipcMain.on('echo', function (event, msg) {
  event.returnValue = msg
})

const coverage = new Coverage({
  outputPath: path.join(__dirname, '..', '..', 'out', 'coverage')
})
coverage.setup()

ipcMain.on('get-main-process-coverage', function (event) {
  event.returnValue = global.__coverage__ || null
})

global.isCi = !!argv.ci
if (global.isCi) {
  process.removeAllListeners('uncaughtException')
  process.on('uncaughtException', function (error) {
    console.error(error, error.stack)
    process.exit(1)
  })
}

// Register app as standard scheme.
global.standardScheme = 'app'
protocol.registerStandardSchemes([global.standardScheme])

app.on('window-all-closed', function () {
  app.quit()
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
    show: !global.isCi,
    width: 800,
    height: 600,
    webPreferences: {
      backgroundThrottling: false
    }
  })
  window.loadURL(url.format({
    pathname: path.join(__dirname, '/index.html'),
    protocol: 'file',
    query: {
      grep: argv.grep,
      invert: argv.invert ? 'true' : ''
    }
  }))
  window.on('unresponsive', function () {
    var chosen = dialog.showMessageBox(window, {
      type: 'warning',
      buttons: ['Close', 'Keep Waiting'],
      message: 'Window is not responsing',
      detail: 'The window is not responding. Would you like to force close it or just keep waiting?'
    })
    if (chosen === 0) window.destroy()
  })

  // For session's download test, listen 'will-download' event in browser, and
  // reply the result to renderer for verifying
  var downloadFilePath = path.join(__dirname, '..', 'fixtures', 'mock.pdf')
  ipcMain.on('set-download-option', function (event, needCancel, preventDefault) {
    window.webContents.session.once('will-download', function (e, item) {
      if (preventDefault) {
        e.preventDefault()
        const url = item.getURL()
        const filename = item.getFilename()
        setImmediate(function () {
          try {
            item.getURL()
          } catch (err) {
            window.webContents.send('download-error', url, filename, err.message)
          }
        })
      } else {
        item.setSavePath(downloadFilePath)
        item.on('done', function (e, state) {
          window.webContents.send('download-done',
            state,
            item.getURL(),
            item.getMimeType(),
            item.getReceivedBytes(),
            item.getTotalBytes(),
            item.getContentDisposition(),
            item.getFilename(),
            item.getSavePath())
        })
        if (needCancel) item.cancel()
      }
    })
    event.returnValue = 'done'
  })

  ipcMain.on('executeJavaScript', function (event, code, hasCallback) {
    if (hasCallback) {
      window.webContents.executeJavaScript(code, (result) => {
        window.webContents.send('executeJavaScript-response', result)
      })
    } else {
      window.webContents.executeJavaScript(code)
      event.returnValue = 'success'
    }
  })
})
