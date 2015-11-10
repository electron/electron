EventEmitter = require('events').EventEmitter
app = require 'app'
ipc = require 'ipc-main'
deprecate = require 'deprecate'

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

  # window.resizeTo(...)
  # window.moveTo(...)
  @webContents.on 'move', (event, size) =>
    @setBounds size

  # Hide the auto-hide menu when webContents is focused.
  @webContents.on 'activate', =>
    if process.platform isnt 'darwin' and @isMenuBarAutoHide() and @isMenuBarVisible()
      @setMenuBarVisibility false

  # Forward the crashed event.
  @webContents.on 'crashed', =>
    @emit 'crashed'

  # Sometimes the webContents doesn't get focus when window is shown, so we have
  # to force focusing on webContents in this case. The safest way is to focus it
  # when we first start to load URL, if we do it earlier it won't have effect,
  # if we do it later we might move focus in the page.
  # Though this hack is only needed on OS X when the app is launched from
  # Finder, we still do it on all platforms in case of other bugs we don't know.
  @webContents.once 'load-url', ->
    @focus()

  # Redirect focus/blur event to app instance too.
  @on 'blur', (event) =>
    app.emit 'browser-window-blur', event, this
  @on 'focus', (event) =>
    app.emit 'browser-window-focus', event, this

  # Notify the creation of the window.
  app.emit 'browser-window-created', {}, this

  # Be compatible with old APIs.
  @webContents.on 'devtools-focused', => @emit 'devtools-focused'
  @webContents.on 'devtools-opened', => @emit 'devtools-opened'
  @webContents.on 'devtools-closed', => @emit 'devtools-closed'
  Object.defineProperty this, 'devToolsWebContents',
    enumerable: true,
    configurable: false,
    get: -> @webContents.devToolsWebContents

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
BrowserWindow::reload = -> @webContents.reload.apply @webContents, arguments
BrowserWindow::send = -> @webContents.send.apply @webContents, arguments
BrowserWindow::openDevTools = -> @webContents.openDevTools.apply @webContents, arguments
BrowserWindow::closeDevTools = -> @webContents.closeDevTools()
BrowserWindow::isDevToolsOpened = -> @webContents.isDevToolsOpened()
BrowserWindow::toggleDevTools = -> @webContents.toggleDevTools()
BrowserWindow::inspectElement = -> @webContents.inspectElement.apply @webContents, arguments
BrowserWindow::inspectServiceWorker = -> @webContents.inspectServiceWorker()

# Deprecated.
deprecate.rename BrowserWindow, 'restart', 'reload'
deprecate.member BrowserWindow, 'undo', 'webContents'
deprecate.member BrowserWindow, 'redo', 'webContents'
deprecate.member BrowserWindow, 'cut', 'webContents'
deprecate.member BrowserWindow, 'copy', 'webContents'
deprecate.member BrowserWindow, 'paste', 'webContents'
deprecate.member BrowserWindow, 'selectAll', 'webContents'
deprecate.member BrowserWindow, 'getUrl', 'webContents'
deprecate.member BrowserWindow, 'reloadIgnoringCache', 'webContents'
deprecate.member BrowserWindow, 'getPageTitle', 'webContents'
deprecate.member BrowserWindow, 'isLoading', 'webContents'
deprecate.member BrowserWindow, 'isWaitingForResponse', 'webContents'
deprecate.member BrowserWindow, 'stop', 'webContents'
deprecate.member BrowserWindow, 'isCrashed', 'webContents'
deprecate.member BrowserWindow, 'executeJavaScriptInDevTools', 'webContents'
deprecate.member BrowserWindow, 'print', 'webContents'
deprecate.member BrowserWindow, 'printToPDF', 'webContents'

module.exports = BrowserWindow
