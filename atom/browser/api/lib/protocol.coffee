app = require 'app'
throw new Error('Can not initialize protocol module before app is ready') unless app.isReady()

protocol = process.atomBinding('protocol').protocol
EventEmitter = require('events').EventEmitter

protocol.__proto__ = EventEmitter.prototype

GetWrappedCallback = (scheme, callback, notification) ->
  wrappedCallback = (error) ->
    if not callback?
      if error
        throw error
      else
        protocol.emit notification, scheme
    else
      callback error, scheme

# Compatibility with old api.
protocol.registerProtocol = (scheme, handler, callback) ->
  protocol._registerProtocol scheme, handler, GetWrappedCallback(scheme, callback, 'registered')

protocol.unregisterProtocol = (scheme, callback) ->
  protocol._unregisterProtocol scheme, GetWrappedCallback(scheme, callback, 'unregistered')

protocol.interceptProtocol = (scheme, handler, callback) ->
  protocol._interceptProtocol scheme, handler, GetWrappedCallback(scheme, callback, 'intercepted')

protocol.uninterceptProtocol = (scheme, callback) ->
  protocol._uninterceptProtocol scheme, GetWrappedCallback(scheme, callback, 'unintercepted')

protocol.RequestStringJob =
class RequestStringJob
  constructor: ({mimeType, charset, data}) ->
    if typeof data isnt 'string' and not data instanceof Buffer
      throw new TypeError('Data should be string or Buffer')

    @mimeType = mimeType ? 'text/plain'
    @charset = charset ? 'UTF-8'
    @data = String data

protocol.RequestBufferJob =
class RequestBufferJob
  constructor: ({mimeType, encoding, data}) ->
    if not data instanceof Buffer
      throw new TypeError('Data should be Buffer')

    @mimeType = mimeType ? 'application/octet-stream'
    @encoding = encoding ? 'utf8'
    @data = new Buffer(data)

protocol.RequestFileJob =
class RequestFileJob
  constructor: (@path) ->

protocol.RequestErrorJob =
class RequestErrorJob
  constructor: (@error) ->

protocol.RequestHttpJob =
class RequestHttpJob
  constructor: ({@session, @url, @method, @referrer}) ->

module.exports = protocol
