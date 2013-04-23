EventEmitter = require('events').EventEmitter
send = process.atom_binding('ipc').send

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit('message', args...)

  send: (args...) ->
    send('ATOM_INTERNAL_MESSAGE', args...)

module.exports = new Ipc
