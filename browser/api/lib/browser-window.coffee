EventEmitter = require('events').EventEmitter
v8Util = process.atomBinding 'v8_util'
objectsRegistry = require '../../atom/objects-registry.js'

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

BrowserWindow::restart = ->
  @loadUrl(@getUrl())

BrowserWindow.getFocusedWindow = ->
  windows = objectsRegistry.getAllWindows()
  return window for window in windows when window.isFocused()

BrowserWindow.fromProcessIdAndRoutingId = (processId, routingId) ->
  windows = objectsRegistry.getAllWindows()
  return window for window in windows when window.getProcessId() == processId and
                                           window.getRoutingId() == routingId

module.exports = BrowserWindow
