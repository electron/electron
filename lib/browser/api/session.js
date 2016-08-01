const {EventEmitter} = require('events')
const {app} = require('electron')
const {fromPartition, _setWrapSession} = process.atomBinding('session')

// Public API.
Object.defineProperties(exports, {
  defaultSession: {
    enumerable: true,
    get () { return fromPartition('') }
  },
  fromPartition: {
    enumerable: true,
    value: fromPartition
  }
})

// Wraps native Session class.
_setWrapSession(function (session) {
  // Session is an EventEmitter.
  Object.setPrototypeOf(session, EventEmitter.prototype)
  app.emit('session-created', session)
})
