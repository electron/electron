const {EventEmitter} = require('events')
const {powerMonitor, PowerMonitor} = process.atomBinding('power_monitor')

// PowerMonitor is an EventEmitter.
Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)
EventEmitter.call(powerMonitor)

module.exports = powerMonitor
