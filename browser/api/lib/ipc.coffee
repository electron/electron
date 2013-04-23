EventEmitter = require('events').EventEmitter
send = process.atom_binding('ipc').send

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit(args...)
    process.on 'ATOM_INTERNAL_MESSAGE_SYNC', (args...) =>
      @emit(args...)

  send: (process_id, routing_id, args...) ->
    @sendChannel(process_id, routing_id, 'message', args...)

  sendChannel: (args...) ->
    send('ATOM_INTERNAL_MESSAGE', args...)

module.exports = new Ipc
