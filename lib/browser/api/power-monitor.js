const EventEmitter = require('events').EventEmitter
const powerMonitor = process.atomBinding('power_monitor').powerMonitor

Object.setPrototypeOf(powerMonitor, EventEmitter.prototype)

module.exports = powerMonitor
