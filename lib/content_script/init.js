'use strict'

/* global nodeProcess, isolatedWorld */

console.log('content script bundle')

const atomBinding = require('@electron/internal/common/atom-binding-setup')(nodeProcess.binding, 'renderer')

const v8Util = atomBinding('v8_util')

const isolatedWorldArgs = v8Util.getHiddenValue(isolatedWorld, 'isolated-world-args')

if (isolatedWorldArgs) {
  const { ipcRenderer, guestInstanceId, isHiddenPage, openerId, usesNativeWindowOpen } = isolatedWorldArgs
  require('@electron/internal/renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen)
}

// require('@electron/internal/renderer/chrome-api').injectTo(window.location.hostname, false, window)
