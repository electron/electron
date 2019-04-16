const ipcRenderer = require('../../renderer/api/ipc-renderer')

const v8Util = process.atomBinding('v8_util')
const ipcNative = process.atomBinding('ipc')

// AtomSandboxedRendererClient will look for the "ipcNative" hidden object when
// invoking the `onMessage`/`onExit` callbacks. We could reuse "ipc" and assign
// `onMessage`/`onExit` directly to `ipcRenderer`, but it is better to separate
// private/public APIs.
v8Util.setHiddenValue(global, 'ipcNative', ipcNative)

ipcNative.onMessage = function (channel, args) {
  ipcRenderer.emit(channel, {sender: ipcRenderer}, ...args)
}

ipcNative.onLoaded = function () {
  process.emit('loaded')
}

ipcNative.onExit = function () {
  process.emit('exit')
}

module.exports = ipcRenderer
