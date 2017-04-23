const {EventEmitter} = require('events')
const {Notification} = process.atomBinding('notification')

Object.setPrototypeOf(Notification.prototype, EventEmitter.prototype)

module.exports = Notification
