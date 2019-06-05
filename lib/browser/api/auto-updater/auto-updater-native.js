'use strict'

const EventEmitter = require('events').EventEmitter
const { autoUpdater, AutoUpdater } = process.electronBinding('auto_updater')

// AutoUpdater is an EventEmitter.
Object.setPrototypeOf(AutoUpdater.prototype, EventEmitter.prototype)
EventEmitter.call(autoUpdater)

module.exports = autoUpdater
