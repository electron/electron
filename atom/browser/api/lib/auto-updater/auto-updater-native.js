var EventEmitter, autoUpdater;

EventEmitter = require('events').EventEmitter;

autoUpdater = process.atomBinding('auto_updater').autoUpdater;

autoUpdater.__proto__ = EventEmitter.prototype;

module.exports = autoUpdater;
