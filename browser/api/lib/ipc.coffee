EventEmitter = require('events').EventEmitter
send = process.atomBinding('ipc').send

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit(args...)
    process.on 'ATOM_INTERNAL_MESSAGE_SYNC', (args...) =>
      @emit(args...)

  send: (processId, routingId, args...) ->
    @sendChannel(processId, routingId, 'message', args...)

  sendChannel: (args...) ->
    send('ATOM_INTERNAL_MESSAGE', args...)

module.exports = new Ipc
