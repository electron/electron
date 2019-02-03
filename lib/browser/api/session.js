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
    cookies.flushStore = deprecate.promisify(cookies.flushStore)
    cookies.get = deprecate.promisify(cookies.get)
    cookies.remove = deprecate.promisify(cookies.remove)
    cookies.set = deprecate.promisify(cookies.set)
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
  this.protocol.isProtocolHandled = deprecate.promisify(this.protocol.isProtocolHandled)
  app.emit('session-created', this)
}
