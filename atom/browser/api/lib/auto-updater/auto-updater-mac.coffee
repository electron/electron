{EventEmitter} = require 'events'
{autoUpdater} = process.atomBinding 'auto_updater'

autoUpdater.__proto__ = EventEmitter.prototype

module.exports = autoUpdater
