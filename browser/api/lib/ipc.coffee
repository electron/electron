EventEmitter = require('events').EventEmitter
send = process.atomBinding('ipc').send

sendWrap = (channel, processId, routingId, args...) ->
  BrowserWindow = require 'browser-window'
  if processId?.constructor is BrowserWindow
    window = processId
    processId = window.getProcessId()
    routingId = window.getRoutingId()

  send channel, processId, routingId, [args...]

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit(args...)
    process.on 'ATOM_INTERNAL_MESSAGE_SYNC', (channel, event, args...) =>
      set = (value) -> event.sendReply JSON.stringify(value)

      Object.defineProperty event, 'returnValue', {set}
      Object.defineProperty event, 'result', {set}

      @emit(channel, event, args...)

  send: (processId, routingId, args...) ->
    @sendChannel(processId, routingId, 'message', args...)

  sendChannel: (args...) ->
    sendWrap('ATOM_INTERNAL_MESSAGE', args...)

module.exports = new Ipc
