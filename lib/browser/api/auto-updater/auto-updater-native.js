const EventEmitter = require('events').EventEmitter
const {autoUpdater, AutoUpdater} = process.atomBinding('auto_updater')

Object.setPrototypeOf(AutoUpdater.prototype, EventEmitter.prototype)

module.exports = autoUpdater
