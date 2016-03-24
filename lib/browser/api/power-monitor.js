const EventEmitter = require('events').EventEmitter
const powerMonitor = process.atomBinding('power_monitor').powerMonitor

powerMonitor.__proto__ = EventEmitter.prototype

module.exports = powerMonitor
