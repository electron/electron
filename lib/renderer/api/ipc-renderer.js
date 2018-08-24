'use strict'

const binding = process.atomBinding('ipc')
const v8Util = process.atomBinding('v8_util')

// Created by init.js.
const ipcRenderer = v8Util.getHiddenValue(global, 'ipc')

ipcRenderer.send = function (...args) {
  return binding.send('ipc-message', args)
}

ipcRenderer.sendSync = function (...args) {
  return binding.sendSync('ipc-message-sync', args)[0]
}

ipcRenderer.sendToHost = function (...args) {
  return binding.send('ipc-message-host', args)
}

ipcRenderer.sendTo = function (webContentsId, channel, ...args) {
  return binding.sendTo(false, webContentsId, channel, args)
}

ipcRenderer.sendToAll = function (webContentsId, channel, ...args) {
  return binding.sendTo(true, webContentsId, channel, args)
}

const removeAllListeners = ipcRenderer.removeAllListeners.bind(ipcRenderer)
ipcRenderer.removeAllListeners = function (...args) {
  if (args.length === 0) {
    throw new Error('Removing all listeners from ipcRenderer will make Electron internals stop working.  Please specify a event name')
  }
  removeAllListeners(...args)
}

module.exports = ipcRenderer
