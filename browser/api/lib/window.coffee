EventEmitter = require('events').EventEmitter

Window = process.atomBinding('window').Window
Window.prototype.__proto__ = EventEmitter.prototype

module.exports = Window
