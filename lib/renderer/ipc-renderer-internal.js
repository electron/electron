'use strict'

const binding = process.atomBinding('ipc')
const v8Util = process.atomBinding('v8_util')

// Created by init.js.
const ipcRenderer = v8Util.getHiddenValue(global, 'ipc-internal')
const internal = true

ipcRenderer.send = function (channel, ...args) {
  return binding.send(internal, channel, args)
}

ipcRenderer.sendSync = function (channel, ...args) {
  return binding.sendSync(internal, channel, args)[0]
}

ipcRenderer.sendTo = function (webContentsId, channel, ...args) {
  return binding.sendTo(internal, false, webContentsId, channel, args)
}

ipcRenderer.sendToAll = function (webContentsId, channel, ...args) {
  return binding.sendTo(internal, true, webContentsId, channel, args)
}

module.exports = ipcRenderer
