'use strict'

const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')

const v8Util = process.atomBinding('v8_util')

const { guestInstanceId, openerId } = process
const hiddenPage = process.argv.includes('--hidden-page')
const usesNativeWindowOpen = process.argv.includes('--native-window-open')
const contextIsolation = process.argv.includes('--context-isolation')

// Pass the arguments to isolatedWorld.
if (contextIsolation) {
  const isolatedWorldArgs = { ipcRenderer, guestInstanceId, hiddenPage, openerId, usesNativeWindowOpen }
  v8Util.setHiddenValue(global, 'isolated-world-args', isolatedWorldArgs)
}

require('@electron/internal/renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage, usesNativeWindowOpen)
