EventEmitter = require('events').EventEmitter
send = process.atomBinding('ipc').send

sendWrap = (channel, processId, routingId, args...) ->
  BrowserWindow = require 'browser-window'
  if processId?.constructor is BrowserWindow
    window = processId
    processId = window.getProcessId()
    routingId = window.getRoutingId()

  send channel, processId, routingId, args...

class Ipc extends EventEmitter
  constructor: ->
    process.on 'ATOM_INTERNAL_MESSAGE', (args...) =>
      @emit(args...)
    process.on 'ATOM_INTERNAL_MESSAGE_SYNC', (channel, event, args...) =>
      returnValue = null
      get = -> returnValue
      set = (value) ->
        throw new Error('returnValue can be only set once') if returnValue?
        returnValue = JSON.stringify(value)
        event.sendReply()

      Object.defineProperty event, 'returnValue', {get, set}
      Object.defineProperty event, 'result', {get, set}

      @emit(channel, event, args...)

  send: (processId, routingId, args...) ->
    @sendChannel(processId, routingId, 'message', args...)

  sendChannel: (args...) ->
    sendWrap('ATOM_INTERNAL_MESSAGE', args...)

module.exports = new Ipc
