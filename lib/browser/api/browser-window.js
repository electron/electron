'use strict'

const {ipcMain} = require('electron')
const {EventEmitter} = require('events')
const {BrowserWindow} = process.atomBinding('window')

Object.setPrototypeOf(BrowserWindow.prototype, EventEmitter.prototype)

BrowserWindow.prototype._init = function () {
  // Avoid recursive require.
  const {app} = require('electron')

  // Simulate the application menu on platforms other than macOS.
  if (process.platform !== 'darwin') {
    const menu = app.getApplicationMenu()
    if (menu) this.setMenu(menu)
  }

  // Make new windows requested by links behave like "window.open"
  this.webContents.on('-new-window', (event, url, frameName, disposition) => {
    const options = {
      show: true,
      width: 800,
      height: 600
    }
    ipcMain.emit('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', event, url, frameName, disposition, options)
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

  // Sometimes the webContents doesn't get focus when window is shown, so we
  // have to force focusing on webContents in this case. The safest way is to
  // focus it when we first start to load URL, if we do it earlier it won't
  // have effect, if we do it later we might move focus in the page.
  //
  // Though this hack is only needed on macOS when the app is launched from
  // Finder, we still do it on all platforms in case of other bugs we don't
  // know.
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
  const visibilityChanged = () => {
    const newState = this.isVisible() && !this.isMinimized()
    if (isVisible !== newState) {
      isVisible = newState
      this.webContents.send('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', isVisible ? 'visible' : 'hidden')
    }
  }

  const visibilityEvents = ['show', 'hide', 'minimize', 'maximize', 'restore']
  for (let event of visibilityEvents) {
    this.on(event, visibilityChanged)
  }

  // Notify the creation of the window.
  app.emit('browser-window-created', {}, this)

  Object.defineProperty(this, 'devToolsWebContents', {
    enumerable: true,
    configurable: false,
    get () {
      return this.webContents.devToolsWebContents
    }
  })
}

BrowserWindow.getFocusedWindow = () => {
  for (let window of BrowserWindow.getAllWindows()) {
    if (window.isFocused()) return window
  }
  return null
}

BrowserWindow.fromWebContents = (webContents) => {
  for (let window of BrowserWindow.getAllWindows()) {
    if (window.webContents.equal(webContents)) return window
  }
}

BrowserWindow.fromDevToolsWebContents = (webContents) => {
  for (let window of BrowserWindow.getAllWindows()) {
    if (window.devToolsWebContents.equal(webContents)) return window
  }
}

// Helpers.
Object.assign(BrowserWindow.prototype, {
  loadURL (...args) {
    return this.webContents.loadURL.apply(this.webContents, args)
  },
  getURL (...args) {
    return this.webContents.getURL()
  },
  reload (...args) {
    return this.webContents.reload.apply(this.webContents, args)
  },
  send (...args) {
    return this.webContents.send.apply(this.webContents, args)
  },
  openDevTools (...args) {
    return this.webContents.openDevTools.apply(this.webContents, args)
  },
  closeDevTools () {
    return this.webContents.closeDevTools()
  },
  isDevToolsOpened () {
    return this.webContents.isDevToolsOpened()
  },
  isDevToolsFocused () {
    return this.webContents.isDevToolsFocused()
  },
  toggleDevTools () {
    return this.webContents.toggleDevTools()
  },
  inspectElement (...args) {
    return this.webContents.inspectElement.apply(this.webContents, args)
  },
  inspectServiceWorker () {
    return this.webContents.inspectServiceWorker()
  },
  showDefinitionForSelection () {
    return this.webContents.showDefinitionForSelection()
  }
})

module.exports = BrowserWindow
