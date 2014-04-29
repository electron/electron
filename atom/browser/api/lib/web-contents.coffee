EventEmitter = require('events').EventEmitter
ipc = require 'ipc'

module.exports.wrap = (webContents) ->
  return null unless webContents.isAlive()

  # webContents is an EventEmitter.
  webContents.__proto__ = EventEmitter.prototype

  # WebContents::send(channel, args..)
  webContents.send = (args...) ->
    @_send 'ATOM_INTERNAL_MESSAGE', [args...]

  # The processId and routingId and identify a webContents.
  webContents.getId = -> "#{@getProcessId()}-#{@getRoutingId()}"
  webContents.equal = (other) -> @getId() is other.getId()

  # Dispatch IPC messages to the ipc module.
  webContents.on 'ipc-message', (event, channel, args...) =>
    Object.defineProperty event, 'sender', value: webContents
    ipc.emit channel, event, args...
  webContents.on 'ipc-message-sync', (event, channel, args...) =>
    Object.defineProperty event, 'returnValue', set: (value) -> event.sendReply JSON.stringify(value)
    Object.defineProperty event, 'sender', value: webContents
    ipc.emit channel, event, args...

  webContents
