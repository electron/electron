const {app, session} = require('electron')
const {registerStandardSchemes} = process.atomBinding('protocol')

exports.registerStandardSchemes = function (schemes) {
  if (app.isReady()) {
    console.warn('protocol.registerStandardSchemes should be called before app is ready')
    return
  }
  registerStandardSchemes(schemes)
}

app.once('ready', function () {
  let protocol = session.defaultSession.protocol
  for (let method in protocol) {
    exports[method] = protocol[method].bind(protocol)
  }
})
