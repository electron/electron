'use strict'

/* eslint no-eval: "off" */
/* global binding, Buffer */
const events = require('events')
const { EventEmitter } = events

process.atomBinding = require('@electron/internal/common/atom-binding-setup').atomBindingSetup(binding.get, 'renderer')

const v8Util = process.atomBinding('v8_util')
// Expose browserify Buffer as a hidden value. This is used by C++ code to
// deserialize Buffer instances sent from browser process.
v8Util.setHiddenValue(global, 'Buffer', Buffer)
// The `lib/renderer/api/ipc-renderer.js` module looks for the ipc object in the
// "ipc" hidden value
v8Util.setHiddenValue(global, 'ipc', new EventEmitter())
// The `lib/renderer/ipc-renderer-internal.js` module looks for the ipc object in the
// "ipc-internal" hidden value
v8Util.setHiddenValue(global, 'ipc-internal', new EventEmitter())
// The process object created by browserify is not an event emitter, fix it so
// the API is more compatible with non-sandboxed renderers.
for (const prop of Object.keys(EventEmitter.prototype)) {
  if (process.hasOwnProperty(prop)) {
    delete process[prop]
  }
}
Object.setPrototypeOf(process, EventEmitter.prototype)

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')

const {
  preloadScripts, isRemoteModuleEnabled, isWebViewTagEnabled, process: processProps
} = ipcRendererInternal.sendSync('ELECTRON_BROWSER_SANDBOX_LOAD')

process.isRemoteModuleEnabled = isRemoteModuleEnabled

// The electron module depends on process.atomBinding
const electron = require('electron')

const loadedModules = new Map([
  ['electron', electron],
  ['events', events],
  ['timers', require('timers')],
  ['url', require('url')]
])

// AtomSandboxedRendererClient will look for the "ipcNative" hidden object when
// invoking the `onMessage`/`onExit` callbacks.
const ipcNative = process.atomBinding('ipc')
v8Util.setHiddenValue(global, 'ipcNative', ipcNative)

ipcNative.onMessage = function (internal, channel, args, senderId) {
  if (internal) {
    ipcRendererInternal.emit(channel, { sender: ipcRendererInternal, senderId }, ...args)
  } else {
    electron.ipcRenderer.emit(channel, { sender: electron.ipcRenderer, senderId }, ...args)
  }
}

ipcNative.onExit = function () {
  process.emit('exit')
}

require('@electron/internal/renderer/web-frame-init')()

// Pass different process object to the preload script(which should not have
// access to things like `process.atomBinding`).
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

process.on('exit', () => preloadProcess.emit('exit'))

// This is the `require` function that will be visible to the preload script
function preloadRequire (module) {
  if (loadedModules.has(module)) {
    return loadedModules.get(module)
  }
  throw new Error('module not found')
}

// Process command line arguments.
const { hasSwitch } = process.atomBinding('command_line')

const isBackgroundPage = hasSwitch('background-page')
const contextIsolation = hasSwitch('context-isolation')

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
}

const guestInstanceId = binding.guestInstanceId && parseInt(binding.guestInstanceId)

// Load webview tag implementation.
if (process.isMainFrame) {
  require('@electron/internal/renderer/web-view/web-view-init')(contextIsolation, isWebViewTagEnabled, guestInstanceId)
}

const errorUtils = require('@electron/internal/common/error-utils')

// Wrap the script into a function executed in global scope. It won't have
// access to the current scope, so we'll expose a few objects as arguments:
//
// - `require`: The `preloadRequire` function
// - `process`: The `preloadProcess` object
// - `Buffer`: Browserify `Buffer` implementation
// - `global`: The window object, which is aliased to `global` by browserify.
//
// Browserify bundles can make use of an external require function as explained
// in https://github.com/substack/node-browserify#multiple-bundles, so electron
// apps can use multi-module preload scripts in sandboxed renderers.
//
// For example, the user can create a bundle with:
//
//     $ browserify -x electron preload.js > renderer.js
//
// and any `require('electron')` calls in `preload.js` will work as expected
// since browserify won't try to include `electron` in the bundle, falling back
// to the `preloadRequire` function above.
function runPreloadScript (preloadSrc) {
  const preloadWrapperSrc = `(function(require, process, Buffer, global, setImmediate, clearImmediate) {
  ${preloadSrc}
  })`

  // eval in window scope
  const preloadFn = binding.createPreloadScript(preloadWrapperSrc)
  const { setImmediate, clearImmediate } = require('timers')

  preloadFn(preloadRequire, preloadProcess, Buffer, global, setImmediate, clearImmediate)
}

for (const { preloadPath, preloadSrc, preloadError } of preloadScripts) {
  try {
    if (preloadSrc) {
      runPreloadScript(preloadSrc)
    } else if (preloadError) {
      throw errorUtils.deserialize(preloadError)
    }
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadPath}`)
    console.error(`${error}`)

    ipcRendererInternal.send('ELECTRON_BROWSER_PRELOAD_ERROR', preloadPath, errorUtils.serialize(error))
  }
}

// Warn about security issues
if (process.isMainFrame) {
  require('@electron/internal/renderer/security-warnings')()
}
