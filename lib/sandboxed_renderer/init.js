/* eslint no-eval: "off" */
/* global binding, preloadPath, process, Buffer */
const events = require('events')

const ipcRenderer = new events.EventEmitter()
const proc = new events.EventEmitter()
// eval in window scope:
// http://www.ecma-international.org/ecma-262/5.1/#sec-10.4.2
const geval = eval

require('../renderer/api/ipc-renderer-setup')(ipcRenderer, binding)

binding.onMessage = function (channel, args) {
  ipcRenderer.emit.apply(ipcRenderer, [channel].concat(args))
}

binding.onExit = function () {
  proc.emit('exit')
}

const preloadModules = new Map([
  ['electron', {
    ipcRenderer: ipcRenderer
  }]
])

function preloadRequire (module) {
  if (preloadModules.has(module)) {
    return preloadModules.get(module)
  }
  throw new Error('module not found')
}

// Fetch the source for the preload
let preloadSrc = ipcRenderer.sendSync('ELECTRON_BROWSER_READ_FILE', preloadPath)
if (preloadSrc.err) {
  throw new Error(preloadSrc.err)
}

// Wrap the source into a function receives a `require` function as argument.
// Browserify bundles can make use of this, as explained in:
// https://github.com/substack/node-browserify#multiple-bundles
//
// For example, the user can create a browserify bundle with:
//
//     $ browserify -x electron preload.js > renderer.js
//
// and any `require('electron')` calls in `preload.js` will work as expected
// since browserify won't try to include `electron` in the bundle and will fall
// back to the `preloadRequire` function above.
let preloadWrapperSrc = `(function(require, process, Buffer, global) {
${preloadSrc.data}
})`

let preloadFn = geval(preloadWrapperSrc)
preloadFn(preloadRequire, proc, Buffer, global)
