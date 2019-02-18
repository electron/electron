'use strict'

/* global nodeProcess, isolatedWorld */

const events = require('events')
const { EventEmitter } = events

process.atomBinding = require('@electron/internal/common/atom-binding-setup')(nodeProcess.binding, 'renderer')

const v8Util = process.atomBinding('v8_util')
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

const isolatedWorldArgs = v8Util.getHiddenValue(isolatedWorld, 'isolated-world-args')

if (isolatedWorldArgs) {
  const { ipcRenderer, guestInstanceId, isHiddenPage, openerId, usesNativeWindowOpen } = isolatedWorldArgs
  require('@electron/internal/renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen)
}

const chromeAPI = require('@electron/internal/renderer/chrome-api')

// Await initialization with extension ID
window.__init = (extensionId) => {
  try {
    chromeAPI.injectTo(extensionId, false, window)
  } catch (e) {
    // Stack trace crashes so we can only log the message
    console.error(e.message)
  }
  delete window.__init
}
