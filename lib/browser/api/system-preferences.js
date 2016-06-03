const {EventEmitter} = require('events')
const {systemPreferences} = process.atomBinding('system_preferences')

Object.setPrototypeOf(systemPreferences, EventEmitter.prototype)

module.exports = systemPreferences
