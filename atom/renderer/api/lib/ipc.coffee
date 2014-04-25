EventEmitter = require('events').EventEmitter
ipc = process.atomBinding('ipc')

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit args...

    window.addEventListener 'unload', (event) ->
      process.removeAllListeners 'ATOM_INTERNAL_MESSAGE'

  send: (args...) ->
    ipc.send 'ipc-message', [args...]

  sendSync: (args...) ->
    JSON.parse ipc.sendSync('ipc-message-sync', [args...])

  # Discarded
  sendChannel: -> @send.apply this, arguments
  sendChannelSync: -> @sendSync.apply this, arguments

module.exports = new Ipc
