EventEmitter = require('events').EventEmitter
BrowserWindow = require 'browser_window'

Menu = process.atomBinding('menu').Menu
Menu::__proto__ = EventEmitter.prototype

popup = Menu::popup
Menu::popup = (window) ->
  throw new TypeError('Invalid window') unless window?.constructor is BrowserWindow

  popup.call this, window

insertSubMenu = Menu::insertSubMenu
Menu::insertSubMenu = (index, command_id, label, submenu) ->
  throw new TypeError('Invalid menu') unless submenu?.constructor is Menu

  @menus = [] unless Array.isArray @menus
  @menus.push submenu # prevent submenu from getting destroyed
  insertSubMenu.apply this, arguments

Menu::appendItem = (args...) -> @insertItem -1, args...
Menu::appendCheckItem = (args...) -> @insertCheckItem -1, args...
Menu::appendRadioItem = (args...) -> @insertRadioItem -1, args...
Menu::appendSeparator = (args...) -> @insertSeparator -1, args...
Menu::appendSubMenu = (args...) -> @insertSubMenu -1, args...

module.exports = Menu
