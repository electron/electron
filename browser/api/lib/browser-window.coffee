EventEmitter = require('events').EventEmitter
app = require 'app'
v8Util = process.atomBinding 'v8_util'

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

BrowserWindow::setMenu = (menu) ->
  throw new Error('BrowserWindow.setMenu is only available on Windows') unless process.platform is 'win32'

  throw new TypeError('Invalid menu') unless menu?.constructor?.name is 'Menu'

  @menu = menu  # Keep a reference of menu in case of GC.
  @menu.attachToWindow this

BrowserWindow.getFocusedWindow = ->
  windows = app.getBrowserWindows()
  return window for window in windows when window.isFocused()

BrowserWindow.fromProcessIdAndRoutingId = (processId, routingId) ->
  windows = app.getBrowserWindows()
  return window for window in windows when window.getProcessId() == processId and
                                           window.getRoutingId() == routingId

module.exports = BrowserWindow
