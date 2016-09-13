const {EventEmitter} = require('events')
const {systemPreferences, SystemPreferences} = process.atomBinding('system_preferences')

Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)

module.exports = systemPreferences
