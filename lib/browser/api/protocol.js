const {app, session} = require('electron')

// Global protocol APIs.
module.exports = process.atomBinding('protocol')

// Fallback protocol APIs of default session.
Object.setPrototypeOf(module.exports, new Proxy({}, {
  get (target, property) {
    if (!app.isReady()) return

    const protocol = session.defaultSession.protocol
    if (!protocol.__proto__.hasOwnProperty(property)) return

    // Returning a native function directly would throw error.
    return (...args) => protocol[property](...args)
  },

  ownKeys () {
    if (!app.isReady()) return []

    return Object.getOwnPropertyNames(session.defaultSession.protocol.__proto__)
  },

  getOwnPropertyDescriptor (target) {
    return { configurable: true, enumerable: true }
  }
}))
