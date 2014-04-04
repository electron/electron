EventEmitter = require('events').EventEmitter
IDWeakMap = require 'id-weak-map'
app = require 'app'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

# Store all created windows in the weak map.
BrowserWindow.windows = new IDWeakMap

BrowserWindow::_init = ->
  # Simulate the application menu on platforms other than OS X.
  if process.platform isnt 'darwin'
    menu = app.getApplicationMenu()
    @setMenu menu if menu?

  # Remember the window.
  id = BrowserWindow.windows.add this

  # Remove the window from weak map immediately when it's destroyed, since we
  # could be iterating windows before GC happended.
  @once 'destroyed', ->
    BrowserWindow.windows.remove id if BrowserWindow.windows.has id

  # Tell the rpc server that a render view has been deleted and we need to
  # release all objects owned by it.
  @on 'render-view-deleted', (event, processId, routingId) ->
    process.emit 'ATOM_BROWSER_RELEASE_RENDER_VIEW', processId, routingId

BrowserWindow::toggleDevTools = ->
  if @isDevToolsOpened() then @closeDevTools() else @openDevTools()

BrowserWindow::restart = ->
  @loadUrl(@getUrl())

BrowserWindow::setMenu = (menu) ->
  if process.platform is 'darwin'
    throw new Error('BrowserWindow.setMenu is not available on OS X')

  throw new TypeError('Invalid menu') unless menu?.constructor?.name is 'Menu'

  @menu = menu  # Keep a reference of menu in case of GC.
  @menu.attachToWindow this

BrowserWindow.getAllWindows = ->
  windows = BrowserWindow.windows
  windows.get key for key in windows.keys()

BrowserWindow.getFocusedWindow = ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.isFocused()

BrowserWindow.fromProcessIdAndRoutingId = (processId, routingId) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.getProcessId() == processId and
                                           window.getRoutingId() == routingId

BrowserWindow.fromDevTools = (processId, routingId) ->
  windows = BrowserWindow.getAllWindows()
  for window in windows
    devtools = window.getDevTools()
    return window if devtools.processId == processId and
                     devtools.routingId == routingId

module.exports = BrowserWindow
