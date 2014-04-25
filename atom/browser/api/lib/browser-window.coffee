EventEmitter = require('events').EventEmitter
IDWeakMap = require 'id-weak-map'
app = require 'app'
ipc = require 'ipc'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

wrapWebContents = (webContents) ->
  return null unless webContents.isAlive()
  # webContents is an EventEmitter.
  webContents.__proto__ = EventEmitter.prototype

  # Wrap around the send method.
  webContents.send = (args...) ->
    @_send 'ATOM_INTERNAL_MESSAGE', [args...]

  # Dispatch the ipc messages.
  webContents.on 'ipc-message', (event, channel, args...) =>
    Object.defineProperty event, 'sender', value: webContents
    ipc.emit channel, event, args...
  webContents.on 'ipc-message-sync', (event, channel, args...) =>
    set = (value) -> event.sendReply JSON.stringify(value)
    Object.defineProperty event, 'returnValue', {set}
    Object.defineProperty event, 'sender', value: webContents
    ipc.emit channel, event, args...

  webContents

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
    process.emit 'ATOM_BROWSER_RELEASE_RENDER_VIEW', processId, routingId

BrowserWindow::toggleDevTools = ->
  if @isDevToolsOpened() then @closeDevTools() else @openDevTools()

BrowserWindow::getWebContents = ->
  wrapWebContents @_getWebContents()

BrowserWindow::getDevToolsWebContents = ->
  wrapWebContents @_getDevToolsWebContents()

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
  for window in windows when window.isDevToolsOpened()
    devtools = window.getDevTools()
    return window if devtools.processId == processId and
                     devtools.routingId == routingId

# Helpers.
BrowserWindow::loadUrl = -> @webContents.loadUrl.apply @webContents, arguments
BrowserWindow::send = -> @webContents.send.apply @webContents, arguments

# Be compatible with old API.
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
BrowserWindow::getDevTools = ->
  processId: @devToolsWebContents.getProcessId()
  routingId: @devToolsWebContents.getRoutingId()
BrowserWindow::executeJavaScriptInDevTools = (code) ->
  @devToolsWebContents.executeJavaScript code

module.exports = BrowserWindow
