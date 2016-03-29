const ipcRenderer = require('electron').ipcRenderer
const deprecate = require('electron').deprecate
const EventEmitter = require('events').EventEmitter

// This module is deprecated, we mirror everything from ipcRenderer.
deprecate.warn('ipc module', 'require("electron").ipcRenderer')

// Routes events of ipcRenderer.
var ipc = new EventEmitter()

ipcRenderer.emit = function (channel, event, ...args) {
  ipc.emit.apply(ipc, [channel].concat(args))
  return EventEmitter.prototype.emit.apply(ipcRenderer, arguments)
}

// Deprecated.
for (var method in ipcRenderer) {
  if (method.startsWith('send')) {
    ipc[method] = ipcRenderer[method]
  }
}

deprecate.rename(ipc, 'sendChannel', 'send')

deprecate.rename(ipc, 'sendChannelSync', 'sendSync')

module.exports = ipc
