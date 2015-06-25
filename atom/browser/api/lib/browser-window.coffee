EventEmitter = require('events').EventEmitter
app = require 'app'
ipc = require 'ipc'

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow::__proto__ = EventEmitter.prototype

BrowserWindow::_init = ->
  # Simulate the application menu on platforms other than OS X.
  if process.platform isnt 'darwin'
    menu = app.getApplicationMenu()
    @setMenu menu if menu?

  # Make new windows requested by links behave like "window.open"
  @webContents.on '-new-window', (event, url, frameName) ->
    options = show: true, width: 800, height: 600
    ipc.emit 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', event, url, frameName, options

  # window.move(...)
  @webContents.on 'move', (event, size) =>
    @setSize size

  # Hide the auto-hide menu when webContents is focused.
  @webContents.on 'activate', =>
    if process.platform isnt 'darwin' and @isMenuBarAutoHide() and @isMenuBarVisible()
      @setMenuBarVisibility false

  # Redirect focus/blur event to app instance too.
  @on 'blur', (event) =>
    app.emit 'browser-window-blur', event, this
  @on 'focus', (event) =>
    app.emit 'browser-window-focus', event, this

BrowserWindow.getFocusedWindow = ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.isFocused()

BrowserWindow.fromWebContents = (webContents) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.webContents?.equal webContents

BrowserWindow.fromDevToolsWebContents = (webContents) ->
  windows = BrowserWindow.getAllWindows()
  return window for window in windows when window.devToolsWebContents?.equal webContents

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
