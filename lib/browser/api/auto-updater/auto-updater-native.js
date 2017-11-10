const EventEmitter = require('events').EventEmitter
const {autoUpdater, AutoUpdater} = process.atomBinding('auto_updater')

// AutoUpdater is an EventEmitter.
Object.setPrototypeOf(AutoUpdater.prototype, EventEmitter.prototype)
EventEmitter.call(autoUpdater)

module.exports = autoUpdater
