const {EventEmitter} = require('events')
const {app} = require('electron')
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

Session.prototype.setCertificateVerifyProc = function (verifyProc) {
  if (verifyProc != null && verifyProc.length > 2) {
    // TODO(kevinsawicki): Remove in 2.0, deprecate before then with warnings
    this._setCertificateVerifyProc(({hostname, certificate, verificationResult}, cb) => {
      verifyProc(hostname, certificate, (result) => {
        // Disabled due to false positive in StandardJS
        // eslint-disable-next-line standard/no-callback-literal
        cb(result ? 0 : -2)
      })
    })
  } else {
    this._setCertificateVerifyProc(verifyProc)
  }
}
