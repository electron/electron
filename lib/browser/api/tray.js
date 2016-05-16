const {EventEmitter} = require('events')
const {Tray} = process.atomBinding('tray')

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

Tray.prototype.setContextMenu = function (menu) {
  this._setContextMenu(menu)

  // Keep a strong reference of menu.
  this.menu = menu
}

module.exports = Tray
