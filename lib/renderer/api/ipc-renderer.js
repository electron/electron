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

module.exports = ipcRenderer
