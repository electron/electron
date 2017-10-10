const {EventEmitter} = require('events')
const {Notification, isSupported, closeAll} = process.atomBinding('notification')

Object.setPrototypeOf(Notification.prototype, EventEmitter.prototype)

Notification.isSupported = isSupported
Notification.closeAll = closeAll

module.exports = Notification
