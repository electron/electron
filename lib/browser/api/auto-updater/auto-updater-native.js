const EventEmitter = require('events').EventEmitter
const autoUpdater = process.atomBinding('auto_updater').autoUpdater

Object.setPrototypeOf(autoUpdater, EventEmitter.prototype)

module.exports = autoUpdater
