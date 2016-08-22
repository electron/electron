const EventEmitter = require('events').EventEmitter

module.exports = new EventEmitter()

// Do not throw exception when channel name is "error".
module.exports.on('error', () => {})
