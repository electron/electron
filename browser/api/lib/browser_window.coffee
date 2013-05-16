EventEmitter = require('events').EventEmitter
objectsRegistry = require '../../atom/objects_registry.js'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow.prototype.__proto__ = EventEmitter.prototype

BrowserWindow.getFocusedWindow = ->
  windows = objectsRegistry.getAllWindows()
  return window for window in windows when window.isFocused()

module.exports = BrowserWindow
