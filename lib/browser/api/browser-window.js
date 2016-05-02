'use strict'

const ipcMain = require('electron').ipcMain
const EventEmitter = require('events').EventEmitter
const {BrowserWindow} = process.atomBinding('window')

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
  this.webContents.on('-new-window', (event, url, frameName, disposition) => {
    var options
    options = {
      show: true,
      width: 800,
      height: 600
    }
    return ipcMain.emit('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', event, url, frameName, disposition, options)
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

  // Subscribe to visibilityState changes and pass to renderer process.
  let isVisible = this.isVisible() && !this.isMinimized()
  let visibilityChanged = () => {
    let newState = this.isVisible() && !this.isMinimized()
    if (isVisible !== newState) {
      isVisible = newState
      this.webContents.send('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', isVisible ? 'visible' : 'hidden')
    }
  }
  this.on('show', visibilityChanged)
  this.on('hide', visibilityChanged)
  this.on('minimize', visibilityChanged)
  this.on('restore', visibilityChanged)
  this.on('maximize', visibilityChanged)

  // Notify the creation of the window.
  app.emit('browser-window-created', {}, this)

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

module.exports = BrowserWindow
