'use strict'

const { EventEmitter } = require('events')
const { systemPreferences, SystemPreferences } = process.electronBinding('system_preferences')
const { markPromisified } = require('@electron/internal/common/promise-utils')

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)
EventEmitter.call(systemPreferences)

module.exports = systemPreferences

// Mark promisifed APIs
markPromisified(systemPreferences.askForMediaAccess)
markPromisified(systemPreferences.promptTouchID)
