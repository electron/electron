import { EventEmitter } from 'events'
const { autoUpdater, AutoUpdater } = process.atomBinding('auto_updater')

// AutoUpdater is an EventEmitter.
Object.setPrototypeOf(AutoUpdater.prototype, EventEmitter.prototype)
EventEmitter.call(autoUpdater as any)

export default autoUpdater
