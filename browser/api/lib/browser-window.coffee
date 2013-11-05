EventEmitter = require('events').EventEmitter
app = require 'app'
v8Util = process.atomBinding 'v8_util'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

BrowserWindow::_init = ->
  # Simulate the application menu on platforms other than OS X.
  if process.platform isnt 'darwin'
    menu = app.getApplicationMenu()
    @setMenu menu if menu?

BrowserWindow::toggleDevTools = ->
  if @isDevToolsOpened() then @closeDevTools() else @openDevTools()

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
