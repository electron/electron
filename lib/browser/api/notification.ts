import { EventEmitter } from 'events'
const { Notification, isSupported } = process.electronBinding('notification')

// Notification is an EventEmitter.
Object.setPrototypeOf(Notification.prototype, EventEmitter.prototype)

Notification.isSupported = isSupported

export = Notification
