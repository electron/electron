'use strict'

// TODO(deepak1556): Deprecate and remove standalone netLog module,
// it is now a property of session module.
const { app, session } = require('electron')

// Fallback to default session.
Object.setPrototypeOf(module.exports, new Proxy({}, {
  get (target, property) {
    if (!app.isReady()) return

    const netLog = session.defaultSession.netLog
    if (!Object.getPrototypeOf(netLog).hasOwnProperty(property)) return

    // Returning a native function directly would throw error.
    return (...args) => netLog[property](...args)
  },

  ownKeys () {
    if (!app.isReady()) return []

    return Object.getOwnPropertyNames(Object.getPrototypeOf(session.defaultSession.netLog))
  },

  getOwnPropertyDescriptor (target) {
    return { configurable: true, enumerable: true }
  }
}))
