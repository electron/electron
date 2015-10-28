app = require 'app'
throw new Error('Can not initialize protocol module before app is ready') unless app.isReady()

protocol = process.atomBinding('protocol').protocol

# Warn about removed APIs.
logAndThrow = (callback, message) ->
  console.error message
  if callback then callback(new Error(message)) else throw new Error(message)
protocol.registerProtocol = (scheme, handler, callback) ->
  logAndThrow callback,
              'registerProtocol API has been replaced by the
               register[File/Http/Buffer/String]Protocol API family, please
               switch to the new APIs.'
protocol.isHandledProtocol = (scheme, callback) ->
  logAndThrow callback,
              'isHandledProtocol API has been replaced by isProtocolHandled.'
protocol.interceptProtocol = (scheme, handler, callback) ->
  logAndThrow callback,
              'interceptProtocol API has been replaced by the
               intercept[File/Http/Buffer/String]Protocol API family, please
               switch to the new APIs.'

module.exports = protocol
