const {app, session} = require('electron')

// Global protocol APIs.
module.exports = process.atomBinding('protocol')

// Fallback protocol APIs of default session.
Object.setPrototypeOf(module.exports, new Proxy({}, {
  get (target, property) {
    if (!app.isReady()) return

    const protocol = session.defaultSession.protocol
    if (!Object.getPrototypeOf(protocol).hasOwnProperty(property)) return

    // Returning a native function directly would throw error.
    return (...args) => protocol[property](...args)
  },

  ownKeys () {
    if (!app.isReady()) return []

    return Object.getOwnPropertyNames(Object.getPrototypeOf(session.defaultSession.protocol))
  },

  getOwnPropertyDescriptor (target) {
    return { configurable: true, enumerable: true }
  }
}))
