const {EventEmitter} = require('events')
const {deprecate} = require('electron')
const {Tray} = process.atomBinding('tray')

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

// TODO(codebytere): remove in 3.0
const nativeSetHighlightMode = Tray.prototype.setHighlightMode
Tray.prototype.setHighlightMode = function (param) {
  if (!process.noDeprecations && typeof param === 'boolean') {
    if (param) {
      deprecate.warn('tray.setHighlightMode(true)', `tray.setHighlightMode("on")`)
    } else {
      deprecate.warn('tray.setHighlightMode(false)', `tray.setHighlightMode("off")`)
    }
  }
  return nativeSetHighlightMode.call(this, param)
}

module.exports = Tray
