EventEmitter = require('events').EventEmitter

Window = process.atom_binding('window').Window
Window.prototype.__proto__ = EventEmitter.prototype

module.exports = Window
