const {EventEmitter} = require('events')
const {deprecate} = require('electron')
const {Tray} = process.atomBinding('tray')

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

Tray.prototype.setHighlightMode = function (param) {
  if (typeof param === 'boolean') {
    if (param) {
      deprecate.warn('tray.setHighlightMode(true)', `tray.setHighlightMode("on")`)
    } else {
      deprecate.warn('tray.setHighlightMode(false)', `tray.setHighlightMode("off")`)
    }
  }
  return this.setHighlightMode(param)
}

module.exports = Tray
