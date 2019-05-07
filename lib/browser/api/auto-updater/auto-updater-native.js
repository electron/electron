'use strict'
const EventEmitter = require('events').EventEmitter
const { autoUpdater, AutoUpdater } = process.electronBinding('auto_updater')
const { deprecate } = require('electron')

// AutoUpdater is an EventEmitter.
Object.setPrototypeOf(AutoUpdater.prototype, EventEmitter.prototype)
EventEmitter.call(autoUpdater)

deprecate.renameFunction(autoUpdater, 'setFeedURL', 'initialize')
deprecate.fnToProperty(autoUpdater, 'feedURL', '_getFeedURL')

module.exports = autoUpdater
