import { EventEmitter } from 'events'
import * as path from 'path'

const Module = require('module')

// Make sure globals like "process" and "global" are always available in preload
// scripts even after they are deleted in "loaded" script.
//
// Note 1: We rely on a Node patch to actually pass "process" and "global" and
// other arguments to the wrapper.
//
// Note 2: Node introduced a new code path to use native code to wrap module
// code, which does not work with this hack. However by modifying the
// "Module.wrapper" we can force Node to use the old code path to wrap module
// code with JavaScript.
//
// Note 3: We provide the equivalent extra variables internally through the
// webpack ProvidePlugin in webpack.config.base.js.  If you add any extra
// variables to this wrapper please ensure to update that plugin as well.
Module.wrapper = [
  '(function (exports, require, module, __filename, __dirname, process, global, Buffer) { ' +
  // By running the code in a new closure, it would be possible for the module
  // code to override "process" and "Buffer" with local variables.
  'return function (exports, require, module, __filename, __dirname) { ',
  '\n}.call(this, exports, require, module, __filename, __dirname); });'
]

// We modified the original process.argv to let node.js load the
// init.js, we need to restore it here.
process.argv.splice(1, 1)

// Clear search paths.

require('../common/reset-search-paths')

// Import common settings.
require('@electron/internal/common/init')

// The global variable will be used by ipc for event dispatching
const v8Util = process.electronBinding('v8_util')

const ipcEmitter = new EventEmitter()
const ipcInternalEmitter = new EventEmitter()
v8Util.setHiddenValue(global, 'ipc', ipcEmitter)
v8Util.setHiddenValue(global, 'ipc-internal', ipcInternalEmitter)

v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal: boolean, channel: string, args: any[], senderId: number) {
    const sender = internal ? ipcInternalEmitter : ipcEmitter
    sender.emit(channel, { sender, senderId }, ...args)
  }
})

// Use electron module after everything is ready.
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils')

// Process command line arguments.
const { getSwitchValue } = process.electronBinding('command_line')

const appPath = getSwitchValue('app-path')

const { webFrameInit } = require('@electron/internal/renderer/web-frame-init')
webFrameInit()

const {
  preloadScripts, contextIsolation, nodeIntegration, nativeWindowOpen, webviewTag, isHiddenPage, guestInstanceId, openerId
} = v8Util.getHiddenValue<Electron.WebPreferencesPayload>(global, 'webPreferences')

// The arguments to be passed to isolated world.
const isolatedWorldArgs = { ipcRendererInternal, guestInstanceId, isHiddenPage, openerId, nativeWindowOpen }

switch (window.location.protocol) {
  case 'devtools:': {
    // Override some inspector APIs.
    require('@electron/internal/renderer/inspector')
    break
  }
  case 'chrome-extension:': {
    // Inject the chrome.* APIs that chrome extensions require
    require('@electron/internal/renderer/chrome-api').injectTo(window.location.hostname, window)
    break
  }
  case 'chrome:':
    break
  default: {
    // Override default web functions.
    const { windowSetup } = require('@electron/internal/renderer/window-setup')
    windowSetup(guestInstanceId, openerId, isHiddenPage, nativeWindowOpen)

    // Inject content scripts.
    const contentScripts = ipcRendererUtils.invokeSync('ELECTRON_GET_CONTENT_SCRIPTS') as Electron.ContentScriptEntry[]
    require('@electron/internal/renderer/content-scripts-injector')(contentScripts)
  }
}

// Load webview tag implementation.
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init')
  webViewInit(contextIsolation, webviewTag, guestInstanceId)
}

// Pass the arguments to isolatedWorld.
if (contextIsolation) {
  v8Util.setHiddenValue(global, 'isolated-world-args', isolatedWorldArgs)
}

if (nodeIntegration) {
  // Export node bindings to global.
  const { makeRequireFunction } = __non_webpack_require__('internal/modules/cjs/helpers') // eslint-disable-line
  global.module = new Module('electron/js2c/renderer_init')
  global.require = makeRequireFunction(global.module)

  // Set the __filename to the path of html file if it is file: protocol.
  if (window.location.protocol === 'file:') {
    const location = window.location
    let pathname = location.pathname

    if (process.platform === 'win32') {
      if (pathname[0] === '/') pathname = pathname.substr(1)

      const isWindowsNetworkSharePath = location.hostname.length > 0 && process.resourcesPath.startsWith('\\')
      if (isWindowsNetworkSharePath) {
        pathname = `//${location.host}/${pathname}`
      }
    }

    global.__filename = path.normalize(decodeURIComponent(pathname))
    global.__dirname = path.dirname(global.__filename)

    // Set module's filename so relative require can work as expected.
    global.module.filename = global.__filename

    // Also search for module under the html file.
    global.module.paths = Module._nodeModulePaths(global.__dirname)
  } else {
    // For backwards compatibility we fake these two paths here
    global.__filename = path.join(process.resourcesPath, 'electron.asar', 'renderer', 'init.js')
    global.__dirname = path.join(process.resourcesPath, 'electron.asar', 'renderer')

    if (appPath) {
      // Search for module under the app directory
      global.module.paths = Module._nodeModulePaths(appPath)
    }
  }

  // Redirect window.onerror to uncaughtException.
  window.onerror = function (_message, _filename, _lineno, _colno, error) {
    if (global.process.listenerCount('uncaughtException') > 0) {
      // We do not want to add `uncaughtException` to our definitions
      // because we don't want anyone else (anywhere) to throw that kind
      // of error.
      global.process.emit('uncaughtException' as any, error as any)
      return true
    } else {
      return false
    }
  }
} else {
  // Delete Node's symbols after the Environment has been loaded in a
  // non context-isolated environment
  if (!contextIsolation) {
    process.once('loaded', function () {
      delete global.process
      delete global.Buffer
      delete global.setImmediate
      delete global.clearImmediate
      delete global.global
    })
  }
}

const errorUtils = require('@electron/internal/common/error-utils')

// Load the preload scripts.
for (const preloadScript of preloadScripts) {
  try {
    Module._load(preloadScript)
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadScript}`)
    console.error(`${error}`)

    ipcRendererInternal.send('ELECTRON_BROWSER_PRELOAD_ERROR', preloadScript, errorUtils.serialize(error))
  }
}

// Warn about security issues
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings')
  securityWarnings(nodeIntegration)
}
