'use strict'

const binding = process.atomBinding('ipc')
const v8Util = process.atomBinding('v8_util')

// Created by init.js.
const ipcRenderer = v8Util.getHiddenValue(global, 'ipc')

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

ipcRenderer.sendToSync = function (webContentsId, channel, ...args) {
  if (typeof webContentsId !== 'number') {
    throw new TypeError('First argument has to be webContentsId')
  }

  const [error, result] = ipcRenderer.sendSync('ELECTRON_BROWSER_SEND_TO_SYNC', webContentsId, channel, ...args)
  if (error) {
    throw new Error(error)
  }
  return result
}

const removeAllListeners = ipcRenderer.removeAllListeners.bind(ipcRenderer)
ipcRenderer.removeAllListeners = function (...args) {
  if (args.length === 0) {
    throw new Error('Removing all listeners from ipcRenderer will make Electron internals stop working.  Please specify a event name')
  }
  removeAllListeners(...args)
}

ipcRenderer.on('ELECTRON_RENDERER_SEND_TO_SYNC', (event, requestId, channel, ...args) => {
  Object.defineProperty(event, 'returnValue', {
    set: (value) => {
      event.sender.send(`ELECTRON_BROWSER_SEND_TO_SYNC_REPLY_${requestId}`, value)
    },
    get: () => undefined
  })
  ipcRenderer.emit(channel, event, ...args)
})

module.exports = ipcRenderer
