'use strict'

const ipcMain = require('electron').ipcMain
const deprecate = require('electron').deprecate
const EventEmitter = require('events').EventEmitter
const {BrowserWindow, _setDeprecatedOptionsCheck} = process.atomBinding('window')

Object.setPrototypeOf(BrowserWindow.prototype, EventEmitter.prototype)

BrowserWindow.prototype._init = function () {
  // avoid recursive require.
  var app, menu
  app = require('electron').app

  // Simulate the application menu on platforms other than OS X.
  if (process.platform !== 'darwin') {
    menu = app.getApplicationMenu()
    if (menu != null) {
      this.setMenu(menu)
    }
  }

  // Make new windows requested by links behave like "window.open"
  this.webContents.on('-new-window', (event, url, frameName) => {
    var options
    options = {
      show: true,
      width: 800,
      height: 600
    }
    return ipcMain.emit('ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_OPEN', event, url, frameName, 'new-window', options)
  })

  // window.resizeTo(...)
  // window.moveTo(...)
  this.webContents.on('move', (event, size) => {
    this.setBounds(size)
  })

  // Hide the auto-hide menu when webContents is focused.
  this.webContents.on('activate', () => {
    if (process.platform !== 'darwin' && this.isMenuBarAutoHide() && this.isMenuBarVisible()) {
      this.setMenuBarVisibility(false)
    }
  })

  // Forward the crashed event.
  this.webContents.on('crashed', () => {
    this.emit('crashed')
  })

  // Change window title to page title.
  this.webContents.on('page-title-updated', (event, title) => {
    // The page-title-updated event is not emitted immediately (see #3645), so
    // when the callback is called the BrowserWindow might have been closed.
    if (this.isDestroyed()) return

    // Route the event to BrowserWindow.
    this.emit('page-title-updated', event, title)
    if (!event.defaultPrevented) this.setTitle(title)
  })

  // Sometimes the webContents doesn't get focus when window is shown, so we have
  // to force focusing on webContents in this case. The safest way is to focus it
  // when we first start to load URL, if we do it earlier it won't have effect,
  // if we do it later we might move focus in the page.
  // Though this hack is only needed on OS X when the app is launched from
  // Finder, we still do it on all platforms in case of other bugs we don't know.
  this.webContents.once('load-url', function () {
    this.focus()
  })

  // Redirect focus/blur event to app instance too.
  this.on('blur', (event) => {
    app.emit('browser-window-blur', event, this)
  })
  this.on('focus', (event) => {
    app.emit('browser-window-focus', event, this)
  })

  // Evented visibilityState changes
  this.on('show', () => {
    this.webContents.send('ATOM_RENDERER_WINDOW_VISIBILITY_CHANGE', this.isVisible(), this.isMinimized())
  })
  this.on('hide', () => {
    this.webContents.send('ATOM_RENDERER_WINDOW_VISIBILITY_CHANGE', this.isVisible(), this.isMinimized())
  })
  this.on('minimize', () => {
    this.webContents.send('ATOM_RENDERER_WINDOW_VISIBILITY_CHANGE', this.isVisible(), this.isMinimized())
  })
  this.on('restore', () => {
    this.webContents.send('ATOM_RENDERER_WINDOW_VISIBILITY_CHANGE', this.isVisible(), this.isMinimized())
  })

  // Notify the creation of the window.
  app.emit('browser-window-created', {}, this)

  // Be compatible with old APIs.
  this.webContents.on('devtools-focused', () => {
    this.emit('devtools-focused')
  })
  this.webContents.on('devtools-opened', () => {
    this.emit('devtools-opened')
  })
  this.webContents.on('devtools-closed', () => {
    this.emit('devtools-closed')
  })
  Object.defineProperty(this, 'devToolsWebContents', {
    enumerable: true,
    configurable: false,
    get: function () {
      return this.webContents.devToolsWebContents
    }
  })
}

BrowserWindow.getFocusedWindow = function () {
  var i, len, window, windows
  windows = BrowserWindow.getAllWindows()
  for (i = 0, len = windows.length; i < len; i++) {
    window = windows[i]
    if (window.isFocused()) {
      return window
    }
  }
  return null
}

BrowserWindow.fromWebContents = function (webContents) {
  var i, len, ref1, window, windows
  windows = BrowserWindow.getAllWindows()
  for (i = 0, len = windows.length; i < len; i++) {
    window = windows[i]
    if ((ref1 = window.webContents) != null ? ref1.equal(webContents) : void 0) {
      return window
    }
  }
}

BrowserWindow.fromDevToolsWebContents = function (webContents) {
  var i, len, ref1, window, windows
  windows = BrowserWindow.getAllWindows()
  for (i = 0, len = windows.length; i < len; i++) {
    window = windows[i]
    if ((ref1 = window.devToolsWebContents) != null ? ref1.equal(webContents) : void 0) {
      return window
    }
  }
}

// Helpers.

BrowserWindow.prototype.loadURL = function () {
  return this.webContents.loadURL.apply(this.webContents, arguments)
}

BrowserWindow.prototype.getURL = function () {
  return this.webContents.getURL()
}

BrowserWindow.prototype.reload = function () {
  return this.webContents.reload.apply(this.webContents, arguments)
}

BrowserWindow.prototype.send = function () {
  return this.webContents.send.apply(this.webContents, arguments)
}

BrowserWindow.prototype.openDevTools = function () {
  return this.webContents.openDevTools.apply(this.webContents, arguments)
}

BrowserWindow.prototype.closeDevTools = function () {
  return this.webContents.closeDevTools()
}

BrowserWindow.prototype.isDevToolsOpened = function () {
  return this.webContents.isDevToolsOpened()
}

BrowserWindow.prototype.isDevToolsFocused = function () {
  return this.webContents.isDevToolsFocused()
}

BrowserWindow.prototype.toggleDevTools = function () {
  return this.webContents.toggleDevTools()
}

BrowserWindow.prototype.inspectElement = function () {
  return this.webContents.inspectElement.apply(this.webContents, arguments)
}

BrowserWindow.prototype.inspectServiceWorker = function () {
  return this.webContents.inspectServiceWorker()
}

// Deprecated.

deprecate.member(BrowserWindow, 'undo', 'webContents')

deprecate.member(BrowserWindow, 'redo', 'webContents')

deprecate.member(BrowserWindow, 'cut', 'webContents')

deprecate.member(BrowserWindow, 'copy', 'webContents')

deprecate.member(BrowserWindow, 'paste', 'webContents')

deprecate.member(BrowserWindow, 'selectAll', 'webContents')

deprecate.member(BrowserWindow, 'reloadIgnoringCache', 'webContents')

deprecate.member(BrowserWindow, 'isLoading', 'webContents')

deprecate.member(BrowserWindow, 'isWaitingForResponse', 'webContents')

deprecate.member(BrowserWindow, 'stop', 'webContents')

deprecate.member(BrowserWindow, 'isCrashed', 'webContents')

deprecate.member(BrowserWindow, 'print', 'webContents')

deprecate.member(BrowserWindow, 'printToPDF', 'webContents')

deprecate.rename(BrowserWindow, 'restart', 'reload')

deprecate.rename(BrowserWindow, 'loadUrl', 'loadURL')

deprecate.rename(BrowserWindow, 'getUrl', 'getURL')

BrowserWindow.prototype.executeJavaScriptInDevTools = deprecate('executeJavaScriptInDevTools', 'devToolsWebContents.executeJavaScript', function (code) {
  var ref1
  return (ref1 = this.devToolsWebContents) != null ? ref1.executeJavaScript(code) : void 0
})

BrowserWindow.prototype.getPageTitle = deprecate('getPageTitle', 'webContents.getTitle', function () {
  var ref1
  return (ref1 = this.webContents) != null ? ref1.getTitle() : void 0
})

const isDeprecatedKey = function (key) {
  return key.indexOf('-') >= 0
}

// Map deprecated key with hyphens to camel case key
const getNonDeprecatedKey = function (deprecatedKey) {
  return deprecatedKey.replace(/-./g, function (match) {
    return match[1].toUpperCase()
  })
}

// TODO Remove for 1.0
const checkForDeprecatedOptions = function (options) {
  if (!options) return ''

  let keysToCheck = Object.keys(options)
  if (options.webPreferences) {
    keysToCheck = keysToCheck.concat(Object.keys(options.webPreferences))
  }

  // Check options for keys with hyphens in them
  let deprecatedKey = keysToCheck.filter(isDeprecatedKey)[0]
  if (deprecatedKey) {
    try {
      deprecate.warn(deprecatedKey, getNonDeprecatedKey(deprecatedKey))
    } catch (error) {
      // Return error message so it can be rethrown via C++
      return error.message
    }
  }

  let webPreferenceOption
  if (options.hasOwnProperty('nodeIntegration')) {
    webPreferenceOption = 'nodeIntegration'
  } else if (options.hasOwnProperty('preload')) {
    webPreferenceOption = 'preload'
  } else if (options.hasOwnProperty('zoomFactor')) {
    webPreferenceOption = 'zoomFactor'
  }
  if (webPreferenceOption) {
    try {
      deprecate.warn(`options.${webPreferenceOption}`, `options.webPreferences.${webPreferenceOption}`)
    } catch (error) {
      // Return error message so it can be rethrown via C++
      return error.message
    }
  }

  return ''
}
_setDeprecatedOptionsCheck(checkForDeprecatedOptions)

module.exports = BrowserWindow
