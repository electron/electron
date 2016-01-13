var EventEmitter, powerMonitor;

EventEmitter = require('events').EventEmitter;

powerMonitor = process.atomBinding('power_monitor').powerMonitor;

powerMonitor.__proto__ = EventEmitter.prototype;

module.exports = powerMonitor;
