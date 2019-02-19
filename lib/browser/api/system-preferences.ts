import { EventEmitter } from 'events'
const { systemPreferences, SystemPreferences } = process.atomBinding('system_preferences')

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)
EventEmitter.call(systemPreferences as any)

export default systemPreferences
