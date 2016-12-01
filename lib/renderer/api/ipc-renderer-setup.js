// Any requires added here need to be added to the browserify_entries array
// in filenames.gypi so they get built into the preload_bundle.js bundle

module.exports = function (ipcRenderer, binding) {
  ipcRenderer.send = function (...args) {
    return binding.send('ipc-message', args)
  }

  ipcRenderer.sendSync = function (...args) {
    return JSON.parse(binding.sendSync('ipc-message-sync', args))
  }

  ipcRenderer.sendToHost = function (...args) {
    return binding.send('ipc-message-host', args)
  }

  ipcRenderer.sendTo = function (webContentsId, channel, ...args) {
    if (typeof webContentsId !== 'number') {
      throw new TypeError('First argument has to be webContentsId')
    }

    ipcRenderer.send('ELECTRON_BROWSER_SEND_TO', false, webContentsId, channel, ...args)
  }

  ipcRenderer.sendToAll = function (webContentsId, channel, ...args) {
    if (typeof webContentsId !== 'number') {
      throw new TypeError('First argument has to be webContentsId')
    }

    ipcRenderer.send('ELECTRON_BROWSER_SEND_TO', true, webContentsId, channel, ...args)
  }

  const removeAllListeners = ipcRenderer.removeAllListeners.bind(ipcRenderer)
  ipcRenderer.removeAllListeners = function (...args) {
    if (args.length === 0) {
      throw new Error('Removing all listeners from ipcRenderer will make Electron internals stop working.  Please specify a event name')
    }
    removeAllListeners(...args)
  }
}
