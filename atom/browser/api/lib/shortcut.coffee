EventEmitter = require('events').EventEmitter
bindings = process.atomBinding 'shortcut'

Shortcut = bindings.Shortcut
Shortcut::__proto__ = EventEmitter.prototype

module.exports = Shortcut
