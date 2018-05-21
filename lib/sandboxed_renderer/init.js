/* eslint no-eval: "off" */
/* global binding, Buffer */
const events = require('events')
const electron = require('electron')

process.atomBinding = require('../common/atom-binding-setup')(binding.get, 'renderer')

const v8Util = process.atomBinding('v8_util')
// Expose browserify Buffer as a hidden value. This is used by C++ code to
// deserialize Buffer instances sent from browser process.
v8Util.setHiddenValue(global, 'Buffer', Buffer)
// The `lib/renderer/api/ipc-renderer.js` module looks for the ipc object in the
// "ipc" hidden value
v8Util.setHiddenValue(global, 'ipc', new events.EventEmitter())
// The process object created by browserify is not an event emitter, fix it so
// the API is more compatible with non-sandboxed renderers.
for (let prop of Object.keys(events.EventEmitter.prototype)) {
  if (process.hasOwnProperty(prop)) {
    delete process[prop]
  }
}
Object.setPrototypeOf(process, events.EventEmitter.prototype)

const remoteModules = new Set([
  'child_process',
  'fs',
  'os',
  'path'
])

const loadedModules = new Map([
  ['electron', electron],
  ['events', events],
  ['timers', require('timers')],
  ['url', require('url')]
])

const {
  preloadSrc, preloadError, platform, env
} = electron.ipcRenderer.sendSync('ELECTRON_BROWSER_SANDBOX_LOAD')

require('../renderer/web-frame-init')()

// Pass different process object to the preload script(which should not have
// access to things like `process.atomBinding`).
const preloadProcess = new events.EventEmitter()
preloadProcess.crash = () => binding.crash()
preloadProcess.hang = () => binding.hang()
preloadProcess.getHeapStatistics = () => binding.getHeapStatistics()
preloadProcess.getProcessMemoryInfo = () => binding.getProcessMemoryInfo()
preloadProcess.getSystemMemoryInfo = () => binding.getSystemMemoryInfo()
preloadProcess.argv = process.argv = binding.getArgv()
preloadProcess.execPath = process.execPath = binding.getExecPath()
preloadProcess.platform = process.platform = platform
preloadProcess.env = process.env = env

process.on('exit', () => preloadProcess.emit('exit'))

// This is the `require` function that will be visible to the preload script
function preloadRequire (module) {
  if (loadedModules.has(module)) {
    return loadedModules.get(module)
  }
  if (remoteModules.has(module)) {
    return require(module)
  }
  throw new Error('module not found')
}

if (window.location.protocol === 'chrome-devtools:') {
  // Override some inspector APIs.
  require('../renderer/inspector')
}

if (binding.guestInstanceId) {
  process.guestInstanceId = parseInt(binding.guestInstanceId)
}

if (!process.guestInstanceId && preloadProcess.argv.indexOf('--webview-tag=true') !== -1) {
  // don't allow recursive `<webview>`
  require('../renderer/web-view/web-view')
  require('../renderer/web-view/web-view-attributes')
}

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
if (preloadSrc) {
  const preloadWrapperSrc = `(function(require, process, Buffer, global, setImmediate, clearImmediate) {
  ${preloadSrc}
  })`

  // eval in window scope
  const preloadFn = binding.createPreloadScript(preloadWrapperSrc)
  const {setImmediate, clearImmediate} = require('timers')
  preloadFn(preloadRequire, preloadProcess, Buffer, global, setImmediate, clearImmediate)
} else if (preloadError) {
  console.error(preloadError.stack)
}
