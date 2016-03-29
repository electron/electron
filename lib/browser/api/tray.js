const deprecate = require('electron').deprecate
const EventEmitter = require('events').EventEmitter
const Tray = process.atomBinding('tray').Tray

Tray.prototype.__proto__ = EventEmitter.prototype

Tray.prototype._init = function () {
  // Deprecated.
  deprecate.rename(this, 'popContextMenu', 'popUpContextMenu')
  deprecate.event(this, 'clicked', 'click')
  deprecate.event(this, 'double-clicked', 'double-click')
  deprecate.event(this, 'right-clicked', 'right-click')
  return deprecate.event(this, 'balloon-clicked', 'balloon-click')
}

Tray.prototype.setContextMenu = function (menu) {
  this._setContextMenu(menu)

  // Keep a strong reference of menu.
  this.menu = menu
}

module.exports = Tray
