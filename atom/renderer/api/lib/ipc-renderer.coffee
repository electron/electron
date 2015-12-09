{EventEmitter} = require 'events'

binding = process.atomBinding 'ipc'
v8Util  = process.atomBinding 'v8_util'

# Created by init.coffee.
ipcRenderer = v8Util.getHiddenValue global, 'ipc'

# Delay the callback to next tick in case the browser is still in the middle
# of sending a message while the callback sends a sync message to browser,
# which can fail sometimes.
ipcRenderer.emit = (args...) ->
  setTimeout (-> EventEmitter::emit.call ipcRenderer, args...), 0

ipcRenderer.send = (args...) ->
  binding.send 'ipc-message', [args...]

ipcRenderer.sendSync = (args...) ->
  JSON.parse binding.sendSync('ipc-message-sync', [args...])

ipcRenderer.sendToHost = (args...) ->
  binding.send 'ipc-message-host', [args...]

module.exports = ipcRenderer
