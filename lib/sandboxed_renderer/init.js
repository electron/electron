'use strict'

/* eslint no-eval: "off" */
/* global binding, Buffer */
const events = require('events')
const { EventEmitter } = events

process.electronBinding = require('@electron/internal/common/electron-binding-setup').electronBindingSetup(binding.get, 'renderer')

const v8Util = process.electronBinding('v8_util')
// Expose Buffer shim as a hidden value. This is used by C++ code to
// deserialize Buffer instances sent from browser process.
v8Util.setHiddenValue(global, 'Buffer', Buffer)
// The `lib/renderer/api/ipc-renderer.ts` module looks for the ipc object in the
// "ipc" hidden value
v8Util.setHiddenValue(global, 'ipc', new EventEmitter())
// The `lib/renderer/ipc-renderer-internal.ts` module looks for the ipc object in the
// "ipc-internal" hidden value
v8Util.setHiddenValue(global, 'ipc-internal', new EventEmitter())
// The process object created by webpack is not an event emitter, fix it so
// the API is more compatible with non-sandboxed renderers.
for (const prop of Object.keys(EventEmitter.prototype)) {
  if (process.hasOwnProperty(prop)) {
    delete process[prop]
  }
}
Object.setPrototypeOf(process, EventEmitter.prototype)

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils')

const {
  contentScripts,
  preloadScripts,
  isRemoteModuleEnabled,
  isWebViewTagEnabled,
  guestInstanceId,
  openerId,
  process: processProps
} = ipcRendererUtils.invokeSync('ELECTRON_BROWSER_SANDBOX_LOAD')

process.isRemoteModuleEnabled = isRemoteModuleEnabled

// The electron module depends on process.electronBinding
const electron = require('electron')

const loadedModules = new Map([
  ['electron', electron],
  ['events', events],
  ['timers', require('timers')],
  ['url', require('url')]
])

// ElectronApiServiceImpl will look for the "ipcNative" hidden object when
// invoking the 'onMessage' callback.
v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal, channel, args, senderId) {
    const sender = internal ? ipcRendererInternal : electron.ipcRenderer
    sender.emit(channel, { sender, senderId }, ...args)
  }
})

// AtomSandboxedRendererClient will look for the "lifecycle" hidden object when
v8Util.setHiddenValue(global, 'lifecycle', {
  onLoaded () {
    process.emit('loaded')
  },
  onExit () {
    process.emit('exit')
  },
  onDocumentStart () {
    process.emit('document-start')
  },
  onDocumentEnd () {
    process.emit('document-end')
  }
})

const { webFrameInit } = require('@electron/internal/renderer/web-frame-init')
webFrameInit()

// Pass different process object to the preload script(which should not have
// access to things like `process.electronBinding`).
const preloadProcess = new EventEmitter()

Object.assign(preloadProcess, binding.process)
Object.assign(preloadProcess, processProps)

Object.assign(process, binding.process)
Object.assign(process, processProps)

Object.defineProperty(preloadProcess, 'noDeprecation', {
  get () {
    return process.noDeprecation
  },
  set (value) {
    process.noDeprecation = value
  }
})

process.on('loaded', () => preloadProcess.emit('loaded'))
process.on('exit', () => preloadProcess.emit('exit'))
process.on('document-start', () => preloadProcess.emit('document-start'))
process.on('document-end', () => preloadProcess.emit('document-end'))

// This is the `require` function that will be visible to the preload script
function preloadRequire (module) {
  if (loadedModules.has(module)) {
    return loadedModules.get(module)
  }
  throw new Error(`module not found: ${module}`)
}

// Process command line arguments.
const { hasSwitch } = process.electronBinding('command_line')

const contextIsolation = hasSwitch('context-isolation')
const isHiddenPage = hasSwitch('hidden-page')
const usesNativeWindowOpen = true

// The arguments to be passed to isolated world.
const isolatedWorldArgs = { ipcRendererInternal, guestInstanceId, isHiddenPage, openerId, usesNativeWindowOpen }

switch (window.location.protocol) {
  case 'devtools:': {
    // Override some inspector APIs.
    require('@electron/internal/renderer/inspector')
    break
  }
  case 'chrome-extension:': {
    // Inject the chrome.* APIs that chrome extensions require
    if (!process.electronBinding('features').isExtensionsEnabled()) {
      require('@electron/internal/renderer/chrome-api').injectTo(window.location.hostname, window)
    }
    break
  }
  case 'chrome': {
    break
  }
  default: {
    // Override default web functions.
    const { windowSetup } = require('@electron/internal/renderer/window-setup')
    windowSetup(guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen)

    // Inject content scripts.
    if (!process.electronBinding('features').isExtensionsEnabled()) {
      require('@electron/internal/renderer/content-scripts-injector')(contentScripts)
    }
  }
}

// Load webview tag implementation.
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init')
  webViewInit(contextIsolation, isWebViewTagEnabled, guestInstanceId)
}

// Pass the arguments to isolatedWorld.
if (contextIsolation) {
  v8Util.setHiddenValue(global, 'isolated-world-args', isolatedWorldArgs)
}

// Wrap the script into a function executed in global scope. It won't have
// access to the current scope, so we'll expose a few objects as arguments:
//
// - `require`: The `preloadRequire` function
// - `process`: The `preloadProcess` object
// - `Buffer`: Shim of `Buffer` implementation
// - `global`: The window object, which is aliased to `global` by webpack.
function runPreloadScript (preloadSrc) {
  const preloadWrapperSrc = `(function(require, process, Buffer, global, setImmediate, clearImmediate, exports) {
  ${preloadSrc}
  })`

  // eval in window scope
  const preloadFn = binding.createPreloadScript(preloadWrapperSrc)
  const { setImmediate, clearImmediate } = require('timers')

  preloadFn(preloadRequire, preloadProcess, Buffer, global, setImmediate, clearImmediate, {})
}

for (const { preloadPath, preloadSrc, preloadError } of preloadScripts) {
  try {
    if (preloadSrc) {
      runPreloadScript(preloadSrc)
    } else if (preloadError) {
      throw preloadError
    }
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadPath}`)
    console.error(error)

    ipcRendererInternal.send('ELECTRON_BROWSER_PRELOAD_ERROR', preloadPath, error)
  }
}

// Warn about security issues
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings')
  securityWarnings()
}
