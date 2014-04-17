powerMonitor = process.atomBinding('power_monitor').powerMonitor
EventEmitter = require('events').EventEmitter

powerMonitor.__proto__ = EventEmitter.prototype

module.exports = powerMonitor
