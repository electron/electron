const {app, session} = require('electron')
const {registerStandardSchemes} = process.atomBinding('protocol')

exports.registerStandardSchemes = registerStandardSchemes

const setupProtocol = function () {
  let protocol = session.defaultSession.protocol
  for (let method in protocol) {
    exports[method] = protocol[method].bind(protocol)
  }
}

if (app.isReady()) {
  setupProtocol()
} else {
  app.once('ready', setupProtocol)
}
