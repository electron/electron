const app = require('electron').app
const {createProtocolObject, registerStandardSchemes} = process.atomBinding('protocol')
let protocol = null

// Warn about removed APIs.
var logAndThrow = function (callback, message) {
  console.error(message)
  if (callback) {
    return callback(new Error(message))
  } else {
    throw new Error(message)
  }
}

exports.registerStandardSchemes = function (schemes) {
  if (app.isReady()) {
    throw new Error('protocol.registerStandardSchemes should be called before app is ready')
  }
  registerStandardSchemes(schemes)
}

app.once('ready', function () {
  protocol = createProtocolObject()
  // Be compatible with old APIs.
  protocol.registerProtocol = function (scheme, handler, callback) {
    return logAndThrow(callback, 'registerProtocol API has been replaced by the register[File/Http/Buffer/String]Protocol API family, please switch to the new APIs.')
  }

  protocol.isHandledProtocol = function (scheme, callback) {
    return logAndThrow(callback, 'isHandledProtocol API has been replaced by isProtocolHandled.')
  }

  protocol.interceptProtocol = function (scheme, handler, callback) {
    return logAndThrow(callback, 'interceptProtocol API has been replaced by the intercept[File/Http/Buffer/String]Protocol API family, please switch to the new APIs.')
  }

  for (let method in protocol) {
    exports[method] = protocol[method].bind(protocol)
  }
})
