const {EventEmitter} = require('events')
const {systemPreferences} = process.atomBinding('system_preferences')

Object.setPrototypeOf(systemPreferences.__proto__, EventEmitter.prototype)

module.exports = systemPreferences
