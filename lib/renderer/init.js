'use strict'

const events = require('events')
const path = require('path')
const Module = require('module')

// We modified the original process.argv to let node.js load the
// atom-renderer.js, we need to restore it here.
process.argv.splice(1, 1)

// Clear search paths.
require('../common/reset-search-paths')

// Import common settings.
require('../common/init')

var globalPaths = Module.globalPaths

// Expose public APIs.
globalPaths.push(path.join(__dirname, 'api', 'exports'))

// The global variable will be used by ipc for event dispatching
var v8Util = process.atomBinding('v8_util')

v8Util.setHiddenValue(global, 'ipc', new events.EventEmitter())

// Use electron module after everything is ready.
const electron = require('electron')

// Call webFrame method.
electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', (event, method, args) => {
  electron.webFrame[method](...args)
})

electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_SYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
  const result = electron.webFrame[method](...args)
  event.sender.send(`ELECTRON_INTERNAL_BROWSER_SYNC_WEB_FRAME_RESPONSE_${requestId}`, result)
})

electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
  const responseCallback = function (result) {
    Promise.resolve(result)
      .then((resolvedResult) => {
        event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, null, resolvedResult)
      })
      .catch((resolvedError) => {
        event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, resolvedError)
      })
  }
  args.push(responseCallback)
  electron.webFrame[method](...args)
})

// Process command line arguments.
let nodeIntegration = 'false'
let preloadScript = null
let isBackgroundPage = false
for (let arg of process.argv) {
  if (arg.indexOf('--guest-instance-id=') === 0) {
    // This is a guest web view.
    process.guestInstanceId = parseInt(arg.substr(arg.indexOf('=') + 1))
  } else if (arg.indexOf('--opener-id=') === 0) {
    // This is a guest BrowserWindow.
    process.openerId = parseInt(arg.substr(arg.indexOf('=') + 1))
  } else if (arg.indexOf('--node-integration=') === 0) {
    nodeIntegration = arg.substr(arg.indexOf('=') + 1)
  } else if (arg.indexOf('--preload=') === 0) {
    preloadScript = arg.substr(arg.indexOf('=') + 1)
  } else if (arg === '--background-page') {
    isBackgroundPage = true
  }
}

if (window.location.protocol === 'chrome-devtools:') {
  // Override some inspector APIs.
  require('./inspector')
  nodeIntegration = 'true'
} else if (window.location.protocol === 'chrome-extension:') {
  // Add implementations of chrome API.
  require('./chrome-api').injectTo(window.location.hostname, isBackgroundPage, window)
  nodeIntegration = 'false'
} else {
  // Override default web functions.
  require('./override')

  // Inject content scripts.
  require('./content-scripts-injector')

  // Load webview tag implementation.
  if (nodeIntegration === 'true' && process.guestInstanceId == null) {
    require('./web-view/web-view')
    require('./web-view/web-view-attributes')
  }
}

if (nodeIntegration === 'true') {
  // Export node bindings to global.
  global.require = require
  global.module = module

  // Set the __filename to the path of html file if it is file: protocol.
  if (window.location.protocol === 'file:') {
    var pathname = process.platform === 'win32' && window.location.pathname[0] === '/' ? window.location.pathname.substr(1) : window.location.pathname
    global.__filename = path.normalize(decodeURIComponent(pathname))
    global.__dirname = path.dirname(global.__filename)

    // Set module's filename so relative require can work as expected.
    module.filename = global.__filename

    // Also search for module under the html file.
    module.paths = module.paths.concat(Module._nodeModulePaths(global.__dirname))
  } else {
    global.__filename = __filename
    global.__dirname = __dirname
  }

  // Redirect window.onerror to uncaughtException.
  window.onerror = function (message, filename, lineno, colno, error) {
    if (global.process.listeners('uncaughtException').length > 0) {
      global.process.emit('uncaughtException', error)
      return true
    } else {
      return false
    }
  }
} else {
  // Delete Node's symbols after the Environment has been loaded.
  process.once('loaded', function () {
    delete global.process
    delete global.setImmediate
    delete global.clearImmediate
    delete global.global
  })
}

// Load the script specfied by the "preload" attribute.
if (preloadScript) {
  try {
    require(preloadScript)
  } catch (error) {
    console.error('Unable to load preload script: ' + preloadScript)
    console.error(error.stack || error.message)
  }
}
