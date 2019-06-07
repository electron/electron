'use strict'

const { EventEmitter } = require('events')
const { deprecate } = require('electron')
const { systemPreferences, SystemPreferences } = process.electronBinding('system_preferences')

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)
EventEmitter.call(systemPreferences)

if ('appLevelAppearance' in systemPreferences) {
  deprecate.fnToProperty(SystemPreferences.prototype, 'appLevelAppearance', '_getAppLevelAppearance', '_setAppLevelAppearance')
}

module.exports = systemPreferences
