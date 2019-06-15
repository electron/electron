'use strict'

/* global nodeProcess, isolatedWorld, worldId */

const { EventEmitter } = require('events')

process.electronBinding = require('@electron/internal/common/electron-binding-setup').electronBindingSetup(nodeProcess._linkedBinding, 'renderer')

const v8Util = process.electronBinding('v8_util')

// The `lib/renderer/ipc-renderer-internal.js` module looks for the ipc object in the
// "ipc-internal" hidden value
v8Util.setHiddenValue(global, 'ipc-internal', v8Util.getHiddenValue(isolatedWorld, 'ipc-internal'))

// The process object created by webpack is not an event emitter, fix it so
// the API is more compatible with non-sandboxed renderers.
for (const prop of Object.keys(EventEmitter.prototype)) {
  if (process.hasOwnProperty(prop)) {
    delete process[prop]
  }
}
Object.setPrototypeOf(process, EventEmitter.prototype)

const isolatedWorldArgs = v8Util.getHiddenValue(isolatedWorld, 'isolated-world-args')

if (isolatedWorldArgs) {
  const { guestInstanceId, isHiddenPage, openerId, nativeWindowOpen } = isolatedWorldArgs
  const { windowSetup } = require('@electron/internal/renderer/window-setup')
  windowSetup(guestInstanceId, openerId, isHiddenPage, nativeWindowOpen)
}

const extensionId = v8Util.getHiddenValue(isolatedWorld, `extension-${worldId}`)

if (extensionId) {
  const chromeAPI = require('@electron/internal/renderer/chrome-api')
  chromeAPI.injectTo(extensionId, window)
}
