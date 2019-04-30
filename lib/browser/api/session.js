'use strict'

const { EventEmitter } = require('events')
const { app, deprecate } = require('electron')
const { fromPartition, Session, Cookies, NetLog, Protocol } = process.electronBinding('session')

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
