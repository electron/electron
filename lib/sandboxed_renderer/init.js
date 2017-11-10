/* eslint no-eval: "off" */
/* global binding, preloadPath, Buffer */
const events = require('events')

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

const electron = require('electron')
const fs = require('fs')
const preloadModules = new Map([
  ['child_process', require('child_process')],
  ['electron', electron],
  ['fs', fs],
  ['os', require('os')],
  ['path', require('path')],
  ['url', require('url')],
  ['timers', require('timers')]
])

const preloadSrc = fs.readFileSync(preloadPath).toString()

// Pass different process object to the preload script(which should not have
// access to things like `process.atomBinding`).
const preloadProcess = new events.EventEmitter()
preloadProcess.crash = () => binding.crash()
preloadProcess.hang = () => binding.hang()
preloadProcess.getProcessMemoryInfo = () => binding.getProcessMemoryInfo()
preloadProcess.getSystemMemoryInfo = () => binding.getSystemMemoryInfo()
preloadProcess.argv = binding.getArgv()
process.platform = preloadProcess.platform = electron.remote.process.platform
process.execPath = preloadProcess.execPath = electron.remote.process.execPath
process.on('exit', () => preloadProcess.emit('exit'))

// This is the `require` function that will be visible to the preload script
function preloadRequire (module) {
  if (preloadModules.has(module)) {
    return preloadModules.get(module)
  }
  throw new Error('module not found')
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
const preloadWrapperSrc = `(function(require, process, Buffer, global, setImmediate) {
${preloadSrc}
})`

// eval in window scope:
// http://www.ecma-international.org/ecma-262/5.1/#sec-10.4.2
const geval = eval
const preloadFn = geval(preloadWrapperSrc)
const {setImmediate} = require('timers')
preloadFn(preloadRequire, preloadProcess, Buffer, global, setImmediate)
