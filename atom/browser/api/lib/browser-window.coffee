EventEmitter = require('events').EventEmitter
IDWeakMap = require 'id-weak-map'
app = require 'app'
ipc = require 'ipc'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

# Store all created windows in the weak map.
BrowserWindow.windows = new IDWeakMap

BrowserWindow::_init = ->
  # Simulate the application menu on platforms other than OS X.
  if process.platform isnt 'darwin'
    menu = app.getApplicationMenu()
    @setMenu menu if menu?

  # Remember the window ID.
  Object.defineProperty this, 'id',
    value: BrowserWindow.windows.add(this)
    enumerable: true

  # Make new windows requested by links behave like "window.open"
  @on '-new-window', (event, url, frameName) =>
    event.sender = @webContents
    options = show: true, width: 800, height: 600
    ipc.emit 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', event, url, frameName, options

  # Redirect "will-navigate" to webContents.
  @on '-will-navigate', (event, url) =>
    @webContents.emit 'will-navigate', event, url

  # Redirect focus/blur event to app instance too.
  @on 'blur', (event) =>
    app.emit 'browser-window-blur', event, this
  @on 'focus', (event) =>
    app.emit 'browser-window-focus', event, this

  # Remove the window from weak map immediately when it's destroyed, since we
  # could be iterating windows before GC happened.
  @once 'closed', =>
    BrowserWindow.windows.remove @id if BrowserWindow.windows.has @id

BrowserWindow::setMenu = (menu) ->
  throw new TypeError('Invalid menu') unless menu is null or menu?.constructor?.name is 'Menu'

  @menu = menu  # Keep a reference of menu in case of GC.
  @_setMenu menu

BrowserWindow.getAllWindows = ->
  windows = BrowserWindow.windows
  windows.get key for key in windows.keys()

BrowserWindow.getFocusedWindow = ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.isFocused()

BrowserWindow.fromWebContents = (webContents) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.webContents?.equal webContents

BrowserWindow.fromDevToolsWebContents = (webContents) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.devToolsWebContents?.equal webContents

BrowserWindow.fromId = (id) ->
  BrowserWindow.windows.get id if BrowserWindow.windows.has id

# Helpers.
BrowserWindow::loadUrl = -> @webContents.loadUrl.apply @webContents, arguments
BrowserWindow::send = -> @webContents.send.apply @webContents, arguments

# Be compatible with old API.
BrowserWindow::restart = -> @webContents.reload()
BrowserWindow::getUrl = -> @webContents.getUrl()
BrowserWindow::reload = -> @webContents.reload.apply @webContents, arguments
BrowserWindow::reloadIgnoringCache = -> @webContents.reloadIgnoringCache.apply @webContents, arguments
BrowserWindow::getPageTitle = -> @webContents.getTitle()
BrowserWindow::isLoading = -> @webContents.isLoading()
BrowserWindow::isWaitingForResponse = -> @webContents.isWaitingForResponse()
BrowserWindow::stop = -> @webContents.stop()
BrowserWindow::isCrashed = -> @webContents.isCrashed()
BrowserWindow::executeJavaScriptInDevTools = (code) -> @devToolsWebContents?.executeJavaScript code
BrowserWindow::openDevTools = -> @webContents.openDevTools.apply @webContents, arguments
BrowserWindow::closeDevTools = -> @webContents.closeDevTools()
BrowserWindow::isDevToolsOpened = -> @webContents.isDevToolsOpened()
BrowserWindow::toggleDevTools = -> @webContents.toggleDevTools()
BrowserWindow::inspectElement = -> @webContents.inspectElement.apply @webContents, arguments
BrowserWindow::inspectServiceWorker = -> @webContents.inspectServiceWorker()
BrowserWindow::print = -> @webContents.print.apply @webContents, arguments
BrowserWindow::printToPDF = -> @webContents.printToPDF.apply @webContents, arguments

module.exports = BrowserWindow
