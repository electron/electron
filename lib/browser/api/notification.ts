import { EventEmitter } from 'events'
const { Notification, isSupported } = process.atomBinding('notification')

Object.setPrototypeOf(Notification.prototype, EventEmitter.prototype)

Notification.isSupported = isSupported

export default Notification
