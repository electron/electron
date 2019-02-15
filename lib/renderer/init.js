'use strict'

const { EventEmitter } = require('events')
const path = require('path')
const Module = require('module')

// We modified the original process.argv to let node.js load the
// init.js, we need to restore it here.
process.argv.splice(1, 1)

// Clear search paths.

require('../common/reset-search-paths')

// Import common settings.
require('@electron/internal/common/init')

const globalPaths = Module.globalPaths

// Expose public APIs.
globalPaths.push(path.join(__dirname, 'api', 'exports'))

// The global variable will be used by ipc for event dispatching
const v8Util = process.atomBinding('v8_util')

v8Util.setHiddenValue(global, 'ipc', new EventEmitter())
v8Util.setHiddenValue(global, 'ipc-internal', new EventEmitter())

// Use electron module after everything is ready.
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')

require('@electron/internal/renderer/web-frame-init')()

// Process command line arguments.
const { hasSwitch, getSwitchValue } = process.atomBinding('command_line')

const parseOption = function (name, defaultValue, converter = value => value) {
  return hasSwitch(name) ? converter(getSwitchValue(name)) : defaultValue
}

const contextIsolation = hasSwitch('context-isolation')
const nodeIntegration = hasSwitch('node-integration')
const webviewTag = hasSwitch('webview-tag')
const isHiddenPage = hasSwitch('hidden-page')
const isBackgroundPage = hasSwitch('background-page')
const usesNativeWindowOpen = hasSwitch('native-window-open')

const preloadScript = parseOption('preload', null)
const preloadScripts = parseOption('preload-scripts', [], value => value.split(path.delimiter))
const appPath = parseOption('app-path', null)
const guestInstanceId = parseOption('guest-instance-id', null, value => parseInt(value))
const openerId = parseOption('opener-id', null, value => parseInt(value))

// The arguments to be passed to isolated world.
const isolatedWorldArgs = { ipcRendererInternal, guestInstanceId, isHiddenPage, openerId, usesNativeWindowOpen }

// The webContents preload script is loaded after the session preload scripts.
if (preloadScript) {
  preloadScripts.push(preloadScript)
}

switch (window.location.protocol) {
  case 'chrome-devtools:': {
    // Override some inspector APIs.
    require('@electron/internal/renderer/inspector')
    break
  }
  case 'chrome-extension:': {
    // Inject the chrome.* APIs that chrome extensions require
    require('@electron/internal/renderer/chrome-api').injectTo(window.location.hostname, isBackgroundPage, window)
    break
  }
  case 'chrome:':
    break
  default: {
    // Override default web functions.
    const { windowSetup } = require('@electron/internal/renderer/window-setup')
    windowSetup(ipcRendererInternal, guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen)

    // Inject content scripts.
    if (process.isMainFrame) {
      require('@electron/internal/renderer/content-scripts-injector')
    }
  }
}

// Load webview tag implementation.
if (process.isMainFrame) {
  require('@electron/internal/renderer/web-view/web-view-init')(contextIsolation, webviewTag, guestInstanceId)
}

// Pass the arguments to isolatedWorld.
if (contextIsolation) {
  v8Util.setHiddenValue(global, 'isolated-world-args', isolatedWorldArgs)
}

if (nodeIntegration) {
  // Export node bindings to global.
  global.require = require
  global.module = module

  // Set the __filename to the path of html file if it is file: protocol.
  if (window.location.protocol === 'file:') {
    const location = window.location
    let pathname = location.pathname

    if (process.platform === 'win32') {
      if (pathname[0] === '/') pathname = pathname.substr(1)

      const isWindowsNetworkSharePath = location.hostname.length > 0 && globalPaths[0].startsWith('\\')
      if (isWindowsNetworkSharePath) {
        pathname = `//${location.host}/${pathname}`
      }
    }

    global.__filename = path.normalize(decodeURIComponent(pathname))
    global.__dirname = path.dirname(global.__filename)

    // Set module's filename so relative require can work as expected.
    module.filename = global.__filename

    // Also search for module under the html file.
    module.paths = module.paths.concat(Module._nodeModulePaths(global.__dirname))
  } else {
    global.__filename = __filename
    global.__dirname = __dirname

    if (appPath) {
      // Search for module under the app directory
      module.paths = module.paths.concat(Module._nodeModulePaths(appPath))
    }
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
    delete global.Buffer
    delete global.setImmediate
    delete global.clearImmediate
    delete global.global
  })
}

const errorUtils = require('@electron/internal/common/error-utils')

// Load the preload scripts.
for (const preloadScript of preloadScripts) {
  try {
    require(preloadScript)
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadScript}`)
    console.error(`${error}`)

    ipcRendererInternal.send('ELECTRON_BROWSER_PRELOAD_ERROR', preloadScript, errorUtils.serialize(error))
  }
}

// Warn about security issues
if (process.isMainFrame) {
  require('@electron/internal/renderer/security-warnings')(nodeIntegration)
}
