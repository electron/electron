import { EventEmitter } from 'events'
const { Tray } = process.electronBinding('tray')

// Tray is an EventEmitter.
Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

export = Tray
