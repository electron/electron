EventEmitter = require('events').EventEmitter

Application = process.atomBinding('app').Application
Application.prototype.__proto__ = EventEmitter.prototype

# Only one App object pemitted.
module.exports = new Application
