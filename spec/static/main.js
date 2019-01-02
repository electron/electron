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
process.crashServicePid = 0

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

// Adding a variable for sandbox process.env test validation
process.env.sandboxmain = ''

// Access console to reproduce #3482.
// eslint-disable-next-line
console

ipcMain.on('message', function (event, ...args) {
  event.sender.send('message', ...args)
})

// Set productName so getUploadedReports() uses the right directory in specs
if (process.platform !== 'darwin') {
  crashReporter.productName = 'Zombies'
}

// Write output to file if OUTPUT_TO_FILE is defined.
const outputToFile = process.env.OUTPUT_TO_FILE
const print = function (_, args) {
  const output = util.format.apply(null, args)
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

global.setTimeoutPromisified = util.promisify(setTimeout)

global.permissionChecks = {
  allow: () => electron.session.defaultSession.setPermissionCheckHandler(null),
  reject: () => electron.session.defaultSession.setPermissionCheckHandler(() => false)
}

global.isCi = !!argv.ci
if (global.isCi) {
  process.removeAllListeners('uncaughtException')
  process.on('uncaughtException', function (error) {
    console.error(error, error.stack)
    process.exit(1)
  })
}

global.nativeModulesEnabled = !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS

// Register app as standard scheme.
global.standardScheme = 'app'
global.zoomScheme = 'zoom'
protocol.registerStandardSchemes([global.standardScheme, global.zoomScheme], { secure: true })

app.on('window-all-closed', function () {
  app.quit()
})

app.on('web-contents-created', (event, contents) => {
  contents.on('crashed', (event, killed) => {
    console.log(`webContents ${contents.id} crashed: ${contents.getURL()} (killed=${killed})`)
  })
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

  // For session's download test, listen 'will-download' event in browser, and
  // reply the result to renderer for verifying
  const downloadFilePath = path.join(__dirname, '..', 'fixtures', 'mock.pdf')
  ipcMain.on('set-download-option', function (event, needCancel, preventDefault, filePath = downloadFilePath, dialogOptions = {}) {
    window.webContents.session.once('will-download', function (e, item) {
      window.webContents.send('download-created',
        item.getState(),
        item.getURLChain(),
        item.getMimeType(),
        item.getReceivedBytes(),
        item.getTotalBytes(),
        item.getFilename(),
        item.getSavePath())
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
        if (item.getState() === 'interrupted' && !needCancel) {
          item.resume()
        } else {
          item.setSavePath(filePath)
          item.setSaveDialogOptions(dialogOptions)
        }
        item.on('done', function (e, state) {
          window.webContents.send('download-done',
            state,
            item.getURL(),
            item.getMimeType(),
            item.getReceivedBytes(),
            item.getTotalBytes(),
            item.getContentDisposition(),
            item.getFilename(),
            item.getSavePath(),
            item.getSaveDialogOptions(),
            item.getURLChain(),
            item.getLastModifiedTime(),
            item.getETag())
        })
        if (needCancel) item.cancel()
      }
    })
    event.returnValue = 'done'
  })

  ipcMain.on('prevent-next-input-event', (event, key, id) => {
    webContents.fromId(id).once('before-input-event', (event, input) => {
      if (key === input.key) event.preventDefault()
    })
    event.returnValue = null
  })

  ipcMain.on('executeJavaScript', function (event, code, hasCallback) {
    let promise

    if (hasCallback) {
      promise = window.webContents.executeJavaScript(code, (result) => {
        window.webContents.send('executeJavaScript-response', result)
      })
    } else {
      promise = window.webContents.executeJavaScript(code)
    }

    promise.then((result) => {
      window.webContents.send('executeJavaScript-promise-response', result)
    }).catch((error) => {
      window.webContents.send('executeJavaScript-promise-error', error)

      if (error && error.name) {
        window.webContents.send('executeJavaScript-promise-error-name', error.name)
      }
    })

    if (!hasCallback) {
      event.returnValue = 'success'
    }
  })
})

ipcMain.on('handle-next-desktop-capturer-get-sources', function (event) {
  event.sender.once('desktop-capturer-get-sources', (event) => {
    event.preventDefault()
  })
})

ipcMain.on('handle-next-remote-require', function (event, modulesMap = {}) {
  event.sender.once('remote-require', (event, moduleName) => {
    event.preventDefault()
    if (modulesMap.hasOwnProperty(moduleName)) {
      event.returnValue = modulesMap[moduleName]
    }
  })
})

ipcMain.on('handle-next-remote-get-global', function (event, globalsMap = {}) {
  event.sender.once('remote-get-global', (event, globalName) => {
    event.preventDefault()
    if (globalsMap.hasOwnProperty(globalName)) {
      event.returnValue = globalsMap[globalName]
    }
  })
})

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

ipcMain.on('close-on-will-navigate', (event, id) => {
  const contents = event.sender
  const window = BrowserWindow.fromId(id)
  window.webContents.once('will-navigate', (event, input) => {
    window.close()
    contents.send('closed-on-will-navigate')
  })
})

ipcMain.on('close-on-will-redirect', (event, id) => {
  const contents = event.sender
  const window = BrowserWindow.fromId(id)
  window.webContents.once('will-redirect', (event, input) => {
    window.close()
    contents.send('closed-on-will-redirect')
  })
})

ipcMain.on('prevent-will-redirect', (event, id) => {
  const window = BrowserWindow.fromId(id)
  window.webContents.once('will-redirect', (event) => {
    event.preventDefault()
  })
})

ipcMain.on('create-window-with-options-cycle', (event) => {
  // This can't be done over remote since cycles are already
  // nulled out at the IPC layer
  const foo = {}
  foo.bar = foo
  foo.baz = {
    hello: {
      world: true
    }
  }
  foo.baz2 = foo.baz
  const window = new BrowserWindow({ show: false, foo: foo })
  event.returnValue = window.id
})

ipcMain.on('prevent-next-new-window', (event, id) => {
  webContents.fromId(id).once('new-window', event => event.preventDefault())
})

ipcMain.on('set-web-preferences-on-next-new-window', (event, id, key, value) => {
  webContents.fromId(id).once('new-window', (event, url, frameName, disposition, options) => {
    options.webPreferences[key] = value
  })
})

ipcMain.on('prevent-next-will-attach-webview', (event) => {
  event.sender.once('will-attach-webview', event => event.preventDefault())
})

ipcMain.on('prevent-next-will-prevent-unload', (event, id) => {
  webContents.fromId(id).once('will-prevent-unload', event => event.preventDefault())
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

ipcMain.on('try-emit-web-contents-event', (event, id, eventName) => {
  const consoleWarn = console.warn
  const contents = webContents.fromId(id)
  const listenerCountBefore = contents.listenerCount(eventName)

  console.warn = (warningMessage) => {
    console.warn = consoleWarn

    const listenerCountAfter = contents.listenerCount(eventName)
    event.returnValue = {
      warningMessage,
      listenerCountBefore,
      listenerCountAfter
    }
  }

  contents.emit(eventName, { sender: contents })
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

ipcMain.on('crash-service-pid', (event, pid) => {
  process.crashServicePid = pid
  event.returnValue = null
})

ipcMain.on('test-webcontents-navigation-observer', (event, options) => {
  let contents = null
  let destroy = () => {}
  if (options.id) {
    const w = BrowserWindow.fromId(options.id)
    contents = w.webContents
    destroy = () => w.close()
  } else {
    contents = webContents.create()
    destroy = () => contents.destroy()
  }

  contents.once(options.name, () => destroy())

  contents.once('destroyed', () => {
    event.sender.send(options.responseEvent)
  })

  contents.loadURL(options.url)
})

ipcMain.on('test-browserwindow-destroy', (event, testOptions) => {
  const focusListener = (event, win) => win.id
  app.on('browser-window-focus', focusListener)
  const windowCount = 3
  const windowOptions = {
    show: false,
    width: 400,
    height: 400,
    webPreferences: {
      backgroundThrottling: false
    }
  }
  const windows = Array.from(Array(windowCount)).map(x => new BrowserWindow(windowOptions))
  windows.forEach(win => win.show())
  windows.forEach(win => win.focus())
  windows.forEach(win => win.destroy())
  app.removeListener('browser-window-focus', focusListener)
  event.sender.send(testOptions.responseEvent)
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
