'use strict'

const { EventEmitter } = require('events')
const { app, deprecate } = require('electron')
const { Session, Cookies } = process.atomBinding('session')
const realFromPartition = process.atomBinding('session').fromPartition

const wrappedSymbol = Symbol('wrapped-deprecate')
const fromPartition = (partition) => {
  const session = realFromPartition(partition)
  if (!session[wrappedSymbol]) {
    session[wrappedSymbol] = true
    const { cookies } = session
    cookies.flushStore = deprecate.promisify(cookies.flushStore, 0)
    cookies.get = deprecate.promisify(cookies.get, 1)
    cookies.remove = deprecate.promisify(cookies.remove, 2)
    cookies.set = deprecate.promisify(cookies.set, 1)
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
  this.protocol.isProtocolHandled = deprecate.promisify(this.protocol.isProtocolHandled, 2)
  app.emit('session-created', this)
}
