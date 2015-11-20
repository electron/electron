binding = process.atomBinding 'ipc'
v8Util  = process.atomBinding 'v8_util'

# Created by init.coffee.
ipcRenderer = v8Util.getHiddenValue global, 'ipc'

ipcRenderer.send = (args...) ->
  binding.send 'ipc-message', [args...]

ipcRenderer.sendSync = (args...) ->
  JSON.parse binding.sendSync('ipc-message-sync', [args...])

ipcRenderer.sendToHost = (args...) ->
  binding.send 'ipc-message-host', [args...]

module.exports = ipcRenderer
