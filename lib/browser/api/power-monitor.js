const {EventEmitter} = require('events')
const {powerMonitor} = process.atomBinding('power_monitor')

Object.setPrototypeOf(powerMonitor.__proto__, EventEmitter.prototype)

module.exports = powerMonitor
