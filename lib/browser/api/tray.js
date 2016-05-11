const EventEmitter = require('events').EventEmitter
const Tray = process.atomBinding('tray').Tray

Object.setPrototypeOf(Tray.prototype, EventEmitter.prototype)

Tray.prototype.setContextMenu = function (menu) {
  this._setContextMenu(menu)

  // Keep a strong reference of menu.
  this.menu = menu
}

module.exports = Tray
