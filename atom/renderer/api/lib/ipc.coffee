EventEmitter = require('events').EventEmitter
ipc = process.atomBinding('ipc')

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit args...

    window.addEventListener 'unload', (event) ->
      process.removeAllListeners 'ATOM_INTERNAL_MESSAGE'

  send: (args...) ->
    @sendChannel 'message', args...

  sendChannel: (args...) ->
    ipc.send 'ATOM_INTERNAL_MESSAGE', [args...]

  sendSync: (args...) ->
    @sendSync 'sync-message', args...

  sendChannelSync: (args...) ->
    JSON.parse ipc.sendSync('ATOM_INTERNAL_MESSAGE_SYNC', [args...])

module.exports = new Ipc
