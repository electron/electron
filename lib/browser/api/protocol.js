const app = require('electron').app

if (!app.isReady()) {
  throw new Error('Can not initialize protocol module before app is ready')
}

module.exports = process.atomBinding('protocol').protocol
