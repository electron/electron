'use strict'

const { EventEmitter } = require('events')
const { app, deprecate } = require('electron')
const { Session, Cookies } = process.atomBinding('session')
const realFromPartition = process.atomBinding('session').fromPartition

const wrappedSymbol = Symbol('wrapped-deprecate')
const fromPartition = (partition) => {
  const session = realFromPartition(partition)
  if (!session[wrappedSymbol]) {
    session.cookies.get = deprecate.promisify(session.cookies.get, 1)
    session.cookies.remove = deprecate.promisify(session.cookies.remove, 2)
    session.cookies.set = deprecate.promisify(session.cookies.set, 1)
    session[wrappedSymbol] = true
  }
  return session
}

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

Object.setPrototypeOf(Session.prototype, EventEmitter.prototype)
Object.setPrototypeOf(Cookies.prototype, EventEmitter.prototype)

Session.prototype._init = function () {
  app.emit('session-created', this)
}
