const {session} = require('electron')

// Global protocol APIs.
module.exports = process.atomBinding('protocol')

// Fallback protocol APIs of default session.
Object.setPrototypeOf(module.exports, new Proxy({}, {
  get (target, property) {
    return (...args) => session.defaultSession.protocol[property](...args)
  },

  ownKeys () {
    return Object.getOwnPropertyNames(session.defaultSession.protocol)
  },

  getOwnPropertyDescriptor (target) {
    return { configurable: true, enumerable: true }
  }
}))
