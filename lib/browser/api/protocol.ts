import { app, session } from 'electron'

// Global protocol APIs.
const protocol = process.electronBinding('protocol')

// Fallback protocol APIs of default session.
Object.setPrototypeOf(protocol, new Proxy({}, {
  get(_target, property) {
    if (app.isReady()) {

      const protocol = session.defaultSession!.protocol
      if (Object.getPrototypeOf(protocol).hasOwnProperty(property)) {

        // Returning a native function directly would throw error.
        return (...args: any[]) => (protocol[property as keyof Electron.Protocol] as Function)(...args)
      }
    }
  },

  ownKeys() {
    if (app.isReady()) return Object.getOwnPropertyNames(Object.getPrototypeOf(session.defaultSession!.protocol))

    return []
  },

  getOwnPropertyDescriptor() {
    return { configurable: true, enumerable: true }
  }
}))

export default protocol
