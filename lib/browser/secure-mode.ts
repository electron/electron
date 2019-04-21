import { app } from 'electron'

function hasListeners (emitter: Electron.EventEmitter | Electron.EventEmitter[], eventName: string) {
  if (Array.isArray(emitter)) {
    return (emitter as Electron.EventEmitter[]).some(item => item.listenerCount(eventName) > 0)
  } else {
    return emitter.listenerCount(eventName) > 0
  }
}

export function preventDefaultInSecureMode (emitter: Electron.EventEmitter | Electron.EventEmitter[], event: Electron.IpcMainEvent, eventName: string) {
  if (app.secureModeEnabled && !hasListeners(emitter, eventName)) {
    console.warn(`Secure mode: preventing default for '${eventName}' event`)
    event.preventDefault()
  }
}
