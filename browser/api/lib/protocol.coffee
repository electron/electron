protocol = process.atomBinding 'protocol'
EventEmitter = require('events').EventEmitter

protocol[key] = value for key, value of EventEmitter.prototype

protocol.RequestStringJob =
class RequestStringJob
  constructor: ({mimeType, charset, data}) ->
    if typeof data isnt 'string' and not data instanceof Buffer
      throw new TypeError('Data should be string or Buffer')

    @mimeType = mimeType ? 'text/plain'
    @charset = charset ? 'UTF-8'
    @data = String data

protocol.RequestFileJob =
class RequestFileJob
  constructor: (@path) ->

module.exports = protocol
