EventEmitter = require('events').EventEmitter
Window = require 'window'

Menu = process.atomBinding('menu').Menu
Menu::__proto__ = EventEmitter.prototype

popup = Menu::popup
Menu::popup = (window) ->
  throw new TypeError('Invalid window') unless window?.constructor is Window

  popup.call this, window

Menu::appendItem = (args...) -> @insertItem -1, args...
Menu::appendCheckItem = (args...) -> @insertCheckItem -1, args...
Menu::appendRadioItem = (args...) -> @insertRadioItem -1, args...
Menu::appendSeparator = (args...) -> @insertSeparator -1, args...
Menu::appendSubMenu = (args...) -> @insertSubMenu -1, args...

module.exports = Menu
