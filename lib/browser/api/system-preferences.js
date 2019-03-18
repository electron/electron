'use strict'

const { EventEmitter } = require('events')
const { systemPreferences, SystemPreferences } = process.electronBinding('system_preferences')

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)
EventEmitter.call(systemPreferences)

module.exports = systemPreferences
