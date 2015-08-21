EventEmitter = require('events').EventEmitter
bindings = process.atomBinding 'tray'

Tray = bindings.Tray
Tray::__proto__ = EventEmitter.prototype

Tray::setContextMenu = (menu) ->
  @_setContextMenu menu
  @menu = menu  # Keep a strong reference of menu.

# Keep compatibility with old APIs.
Tray::popContextMenu = Tray::popUpContextMenu

module.exports = Tray
