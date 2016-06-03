const {EventEmitter} = require('events')
const {powerMonitor} = process.atomBinding('power_monitor')

Object.setPrototypeOf(powerMonitor, EventEmitter.prototype)

module.exports = powerMonitor
