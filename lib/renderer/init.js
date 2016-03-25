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

if (!process.env.ELECTRON_HIDE_INTERNAL_MODULES) {
  globalPaths.push(path.join(__dirname, 'api'))
}

// Expose public APIs.
globalPaths.push(path.join(__dirname, 'api', 'exports'))

// The global variable will be used by ipc for event dispatching
var v8Util = process.atomBinding('v8_util')

v8Util.setHiddenValue(global, 'ipc', new events.EventEmitter)

// Use electron module after everything is ready.
const electron = require('electron')

// Call webFrame method.
electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', (event, method, args) => {
  electron.webFrame[method].apply(electron.webFrame, args)
})

electron.ipcRenderer.on('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', (event, requestId, method, args) => {
  const responseCallback = function (result) {
    event.sender.send(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, result)
  }
  args.push(responseCallback)
  electron.webFrame[method].apply(electron.webFrame, args)
})

// Process command line arguments.
var nodeIntegration = 'false'
var preloadScript = null

var ref = process.argv
var i, len, arg
for (i = 0, len = ref.length; i < len; i++) {
  arg = ref[i]
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
  }
}

if (location.protocol === 'chrome-devtools:') {
  // Override some inspector APIs.
  require('./inspector')
  nodeIntegration = 'true'
} else if (location.protocol === 'chrome-extension:') {
//   // Add implementations of chrome API.
// TODO(bridiver) - get rid of chrome-api
//   require('./chrome-api');
  nodeIntegration = 'false'
} else {
  // Override default web functions.
  require('./override')

  // Load webview tag implementation.
  if (process.guestInstanceId == null) {
    require('./web-view/web-view')
    require('./web-view/web-view-attributes')
  }
}

if (nodeIntegration === 'true' || nodeIntegration === 'all' || nodeIntegration === 'except-iframe' || nodeIntegration === 'manual-enable-iframe') {
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
    return delete global.global
  })
}

// Load the script specfied by the "preload" attribute.
if (preloadScript) {
  try {
    require(preloadScript)
  } catch (error) {
    if (error.code === 'MODULE_NOT_FOUND') {
      console.error('Unable to load preload script ' + preloadScript)
    } else {
      console.error(error)
      console.error(error.stack)
    }
  }
}
