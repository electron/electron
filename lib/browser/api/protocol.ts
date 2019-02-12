import { app, session } from 'electron'

// Global protocol APIs.
const protocol = process.atomBinding('protocol')

// Fallback protocol APIs of default session.
Object.setPrototypeOf(protocol, new Proxy({}, {
  get (target, property) {
    if (!app.isReady()) return

    const protocol = session.defaultSession!.protocol
    if (!Object.getPrototypeOf(protocol).hasOwnProperty(property)) return

    // Returning a native function directly would throw error.
    return (...args: any[]) => (protocol[property as keyof Electron.Protocol] as Function)(...args)
  },

  ownKeys () {
    if (!app.isReady()) return []

    return Object.getOwnPropertyNames(Object.getPrototypeOf(session.defaultSession!.protocol))
  },

  getOwnPropertyDescriptor (target) {
    return { configurable: true, enumerable: true }
  }
}))

export default protocol
