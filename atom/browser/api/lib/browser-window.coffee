EventEmitter = require('events').EventEmitter
IDWeakMap = require 'id-weak-map'
app = require 'app'
wrapWebContents = require('web-contents').wrap

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

# Store all created windows in the weak map.
BrowserWindow.windows = new IDWeakMap

BrowserWindow::_init = ->
  # Simulate the application menu on platforms other than OS X.
  if process.platform isnt 'darwin'
    menu = app.getApplicationMenu()
    @setMenu menu if menu?

  @webContents = @getWebContents()
  @webContents.once 'destroyed', => @webContents = null

  # Define getter for devToolsWebContents.
  devToolsWebContents = null
  @__defineGetter__ 'devToolsWebContents', ->
    if @isDevToolsOpened()
      # Get a new devToolsWebContents if previous one has been destroyed, it
      # could happen when the devtools has been closed and then reopened.
      devToolsWebContents = null unless devToolsWebContents?.isAlive()
      devToolsWebContents ?= @getDevToolsWebContents()
    else
      devToolsWebContents = null

  # Remember the window.
  id = BrowserWindow.windows.add this

  # Remove the window from weak map immediately when it's destroyed, since we
  # could be iterating windows before GC happended.
  @once 'closed', ->
    BrowserWindow.windows.remove id if BrowserWindow.windows.has id

  # Tell the rpc server that a render view has been deleted and we need to
  # release all objects owned by it.
  @webContents.on 'render-view-deleted', (event, processId, routingId) ->
    process.emit 'ATOM_BROWSER_RELEASE_RENDER_VIEW', "#{processId}-#{routingId}"

BrowserWindow::toggleDevTools = ->
  if @isDevToolsOpened() then @closeDevTools() else @openDevTools()

BrowserWindow::getWebContents = ->
  wrapWebContents @_getWebContents()

BrowserWindow::getDevToolsWebContents = ->
  wrapWebContents @_getDevToolsWebContents()

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

BrowserWindow.fromWebContents = (webContents) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when webContents.equal window.webContents

BrowserWindow.fromDevToolsWebContents = (webContents) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when webContents.equal window.devToolsWebContents

# Helpers.
BrowserWindow::loadUrl = -> @webContents.loadUrl.apply @webContents, arguments
BrowserWindow::send = -> @webContents.send.apply @webContents, arguments

# Be compatible with old API.
BrowserWindow::restart = -> @webContents.restart()
BrowserWindow::getUrl = -> @webContents.getUrl()
BrowserWindow::reload = -> @webContents.reload()
BrowserWindow::reloadIgnoringCache = -> @webContents.reloadIgnoringCache()
BrowserWindow::getPageTitle = -> @webContents.getTitle()
BrowserWindow::isLoading = -> @webContents.isLoading()
BrowserWindow::isWaitingForResponse = -> @webContents.isWaitingForResponse()
BrowserWindow::stop = -> @webContents.stop()
BrowserWindow::getRoutingId = -> @webContents.getRoutingId()
BrowserWindow::getProcessId = -> @webContents.getProcessId()
BrowserWindow::isCrashed = -> @webContents.isCrashed()
BrowserWindow::executeJavaScriptInDevTools = (code) ->
  @devToolsWebContents.executeJavaScript code

module.exports = BrowserWindow
