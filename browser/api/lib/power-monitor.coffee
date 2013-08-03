bindings = process.atomBinding 'power_monitor'
EventEmitter = require('events').EventEmitter

PowerMonitor = bindings.PowerMonitor
PowerMonitor::__proto__ = EventEmitter.prototype

module.exports = new PowerMonitor
