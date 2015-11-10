deprecate = require 'deprecate'
ipcRenderer = require 'ipc-renderer'
{EventEmitter} = require 'events'

# This module is deprecated, we mirror everything from ipcRenderer.
deprecate.warn 'ipc module', 'ipcRenderer module'

# Routes events of ipcRenderer.
ipc = new EventEmitter
ipcRenderer.emit = (channel, event, args...) ->
  ipc.emit channel, args...
  EventEmitter::emit.apply ipcRenderer, arguments

# Deprecated.
for method of ipcRenderer when method.startsWith 'send'
  ipc[method] = ipcRenderer[method]
deprecate.rename ipc, 'sendChannel', 'send'
deprecate.rename ipc, 'sendChannelSync', 'sendSync'

module.exports = ipc
