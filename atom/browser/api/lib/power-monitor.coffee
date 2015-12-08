{EventEmitter} = require 'events'

{powerMonitor} = process.atomBinding 'power_monitor'

powerMonitor.__proto__ = EventEmitter.prototype

module.exports = powerMonitor
