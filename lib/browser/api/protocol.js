const {app} = require('electron')
const {createProtocolObject, registerStandardSchemes} = process.atomBinding('protocol')

exports.registerStandardSchemes = function (schemes) {
  if (app.isReady()) {
    console.warn('protocol.registerStandardSchemes should be called before app is ready')
    return
  }
  registerStandardSchemes(schemes)
}

app.once('ready', function () {
  let protocol = createProtocolObject()
  for (let method in protocol) {
    exports[method] = protocol[method].bind(protocol)
  }
})
