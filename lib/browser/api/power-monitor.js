const {EventEmitter} = require('events')
const {powerMonitor, PowerMonitor} = process.atomBinding('power_monitor')

Object.setPrototypeOf(PowerMonitor.prototype, EventEmitter.prototype)

module.exports = powerMonitor
