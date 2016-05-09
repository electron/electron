const app = require('electron').app
const {createProtocolObject, registerStandardSchemes} = process.atomBinding('protocol')

exports.registerStandardSchemes = function (schemes) {
  if (app.isReady()) {
    throw new Error('protocol.registerStandardSchemes should be called before app is ready')
  }
  registerStandardSchemes(schemes)
}

app.once('ready', function () {
  let protocol = createProtocolObject()
  for (let method in protocol) {
    exports[method] = protocol[method].bind(protocol)
  }
})
