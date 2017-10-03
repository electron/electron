const {EventEmitter} = require('events')
const {Notification, isSupported} = process.atomBinding('notification')

Object.setPrototypeOf(Notification.prototype, EventEmitter.prototype)

Notification.isSupported = isSupported

module.exports = Notification
