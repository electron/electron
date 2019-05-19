import { EventEmitter } from 'events'
import { createLazyInstance } from '../utils'

const { Screen, createScreen } = process.electronBinding('screen')

// Screen is an EventEmitter.
Object.setPrototypeOf(Screen.prototype, EventEmitter.prototype)

export = createLazyInstance(createScreen, Screen, true) as Electron.Screen
