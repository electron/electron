'use strict'

const { EventEmitter } = require('events')
const { app, deprecate } = require('electron')
const { fromPartition, Session, Cookies, NetLog, Protocol } = process.atomBinding('session')

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

Session.prototype.clearStorageData = deprecate.promisify(Session.prototype.clearStorageData)

Cookies.prototype.flushStore = deprecate.promisify(Cookies.prototype.flushStore)
Cookies.prototype.get = deprecate.promisify(Cookies.prototype.get)
Cookies.prototype.remove = deprecate.promisify(Cookies.prototype.remove)
Cookies.prototype.set = deprecate.promisify(Cookies.prototype.set)

NetLog.prototype.stopLogging = deprecate.promisify(NetLog.prototype.stopLogging)

Protocol.prototype.isProtocolHandled = deprecate.promisify(Protocol.prototype.isProtocolHandled)
