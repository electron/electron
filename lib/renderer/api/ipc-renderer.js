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
    throw new TypeError(`First argument has to be webContentsId`)
  }

  ipcRenderer.send('ELECTRON_BROWSER_SEND_TO', false, webContentsId, channel, ...args)
}

ipcRenderer.sendToAll = function (webContentsId, channel, ...args) {
  if (typeof webContentsId !== 'number') {
    throw new TypeError(`First argument has to be webContentsId`)
  }

  ipcRenderer.send('ELECTRON_BROWSER_SEND_TO', true, webContentsId, channel, ...args)
}

module.exports = ipcRenderer
