import { EventEmitter } from 'events'

const emitter = new EventEmitter()

// Do not throw exception when channel name is "error".
emitter.on('error', () => {})

export const ipcMainInternal = emitter
