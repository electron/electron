import { EventEmitter } from 'events'
import { app } from 'electron'

const { fromPartition, Session, Cookies } = process.electronBinding('session')

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

// Session is an EventEmitter.
Object.setPrototypeOf(Session.prototype, EventEmitter.prototype)

// Cookies is an EventEmitter.
Object.setPrototypeOf(Cookies.prototype, EventEmitter.prototype)

Session.prototype._init = function () {
  app.emit('session-created', this)
}

const _originalStartLogging = NetLog.prototype.startLogging
NetLog.prototype.startLogging = function (path) {
  this._currentlyLoggingPath = path
  try {
    return _originalStartLogging.call(this, path)
  } catch (e) {
    this._currentlyLoggingPath = null
    throw e
  }
}

const _originalStopLogging = NetLog.prototype.stopLogging
NetLog.prototype.stopLogging = function () {
  this._currentlyLoggingPath = null
  return _originalStopLogging.call(this)
}

const currentlyLoggingPathDeprecated = deprecate.warnOnce('currentlyLoggingPath')
Object.defineProperties(NetLog.prototype, {
  currentlyLoggingPath: {
    enumerable: true,
    get () {
      currentlyLoggingPathDeprecated()
      return this._currentlyLoggingPath == null ? '' : this._currentlyLoggingPath
    }
  }
})
