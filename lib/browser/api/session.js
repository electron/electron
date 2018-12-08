'use strict'

const { EventEmitter } = require('events')
const { app, deprecate } = require('electron')
const { fromPartition, Session, Cookies } = process.atomBinding('session')

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

const nativeCookieSet = Cookies.set
Cookies.set = function (details, callback) {
  return deprecate.promisify(nativeCookieSet.call(this, details), callback)
}

Session.prototype._init = function () {
  app.emit('session-created', this)
}
