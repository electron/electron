EventEmitter = require('events').EventEmitter
send = process.atom_binding('ipc').send

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit('message', args...)

  send: (process_id, routing_id, args...) ->
    @sendChannel(process_id, routing_id, 'ATOM_INTERNAL_MESSAGE', args...)

  sendChannel: (args...) ->
    send(args...)

module.exports = new Ipc
