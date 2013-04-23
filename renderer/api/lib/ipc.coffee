EventEmitter = require('events').EventEmitter
send = process.atom_binding('ipc').send

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit(args...)

  send: (args...) ->
    @sendChannel('message', args...)

  sendChannel: (args...) ->
    send('ATOM_INTERNAL_MESSAGE', args...)

module.exports = new Ipc
