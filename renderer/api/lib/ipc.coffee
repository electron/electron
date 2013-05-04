EventEmitter = require('events').EventEmitter
ipc = process.atomBinding('ipc')

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit(args...)

    window.addEventListener 'unload', (event) ->
      process.removeAllListeners 'ATOM_INTERNAL_MESSAGE'

  send: (args...) ->
    ipc.send('ATOM_INTERNAL_MESSAGE', 'message', args...)

  sendChannel: (args...) ->
    ipc.send('ATOM_INTERNAL_MESSAGE', args...)

  sendSync: (args...) ->
    ipc.sendSync('ATOM_INTERNAL_MESSAGE_SYNC', 'sync-message', args...).result

  sendChannelSync: (args...) ->
    ipc.sendSync('ATOM_INTERNAL_MESSAGE_SYNC', args...).result

module.exports = new Ipc
