const {EventEmitter} = require('events')
const {app, deprecate} = require('electron')
const {fromPartition, Session, Cookies} = process.atomBinding('session')

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

// Remove after 2.0
Session.prototype.setCertificateVerifyProc = function (verifyProc) {
  if (!verifyProc) {
    this._setCertificateVerifyProc(null)
    return
  }
  if (verifyProc.length <= 3) {
    deprecate.warn('setCertificateVerifyproc(hostname, certificate, callback)',
      'setCertificateVerifyproc(hostname, certificate, error, callback)')
    this._setCertificateVerifyProc((hostname, certificate, error, cb) => {
      verifyProc(hostname, certificate, (result) => {
        cb(result ? 0 : -2)
      })
    })
  } else {
    this._setCertificateVerifyProc(verifyProc)
  }
}
