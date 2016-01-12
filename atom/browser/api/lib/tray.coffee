{deprecate} = require 'electron'
{EventEmitter} = require 'events'

{Tray} = process.atomBinding 'tray'
Tray::__proto__ = EventEmitter.prototype

Tray::_init = ->
  ### Deprecated. ###
  deprecate.rename this, 'popContextMenu', 'popUpContextMenu'
  deprecate.event this, 'clicked', 'click'
  deprecate.event this, 'double-clicked', 'double-click'
  deprecate.event this, 'right-clicked', 'right-click'
  deprecate.event this, 'balloon-clicked', 'balloon-click'

Tray::setContextMenu = (menu) ->
  @_setContextMenu menu
  ### Keep a strong reference of menu. ###
  @menu = menu

module.exports = Tray
