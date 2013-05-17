EventEmitter = require('events').EventEmitter
v8Util = process.atomBinding 'v8_util'
objectsRegistry = require '../../atom/objects_registry.js'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

BrowserWindow::toggleDevTools = ->
  opened = v8Util.getHiddenValue this, 'devtoolsOpened'
  if opened
    @closeDevTools()
    v8Util.setHiddenValue this, 'devtoolsOpened', false
  else
    @openDevTools()
    v8Util.setHiddenValue this, 'devtoolsOpened', true

BrowserWindow.getFocusedWindow = ->
  windows = objectsRegistry.getAllWindows()
  return window for window in windows when window.isFocused()

module.exports = BrowserWindow
