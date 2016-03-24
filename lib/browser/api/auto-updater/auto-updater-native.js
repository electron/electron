const EventEmitter = require('events').EventEmitter
const autoUpdater = process.atomBinding('auto_updater').autoUpdater

autoUpdater.__proto__ = EventEmitter.prototype

module.exports = autoUpdater
