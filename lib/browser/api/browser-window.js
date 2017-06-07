'use strict'

const electron = require('electron')
const {ipcMain} = electron
const {EventEmitter} = require('events')
const {BrowserWindow} = process.atomBinding('window')
const v8Util = process.atomBinding('v8_util')

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
  this.webContents.on('-new-window', (event, url, frameName,
                                      disposition, additionalFeatures,
                                      postData) => {
    const options = {
      show: true,
      width: 800,
      height: 600
    }
    ipcMain.emit('ELECTRON_GUEST_WINDOW_MANAGER_INTERNAL_WINDOW_OPEN',
                 event, url, frameName, disposition,
                 options, additionalFeatures, postData)
  })

  this.webContents.on('-web-contents-created', (event, webContents, url,
                                                frameName) => {
    v8Util.setHiddenValue(webContents, 'url-framename', {url, frameName})
  })

  // Create a new browser window for the native implementation of
  // "window.open", used in sandbox and nativeWindowOpen mode
  this.webContents.on('-add-new-contents', (event, webContents, disposition,
                                            userGesture, left, top, width,
                                            height) => {
    let urlFrameName = v8Util.getHiddenValue(webContents, 'url-framename')
    if ((disposition !== 'foreground-tab' && disposition !== 'new-window') ||
        !urlFrameName) {
      return
    }

    let {url, frameName} = urlFrameName
    v8Util.deleteHiddenValue(webContents, 'url-framename')
    const options = {
      show: true,
      x: left,
      y: top,
      width: width || 800,
      height: height || 600,
      webContents: webContents
    }
    ipcMain.emit('ELECTRON_GUEST_WINDOW_MANAGER_INTERNAL_WINDOW_OPEN',
                 event, url, frameName, disposition, options)
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
    // Route the event to BrowserWindow.
    this.emit('page-title-updated', event, title)
    if (!this.isDestroyed() && !event.defaultPrevented) this.setTitle(title)
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
      const visibilityState = isVisible ? 'visible' : 'hidden'
      this.webContents.emit('-window-visibility-change', visibilityState)
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
  for (const window of BrowserWindow.getAllWindows()) {
    if (window.webContents.equal(webContents)) return window
  }
}

BrowserWindow.fromDevToolsWebContents = (webContents) => {
  for (const window of BrowserWindow.getAllWindows()) {
    const {devToolsWebContents} = window
    if (devToolsWebContents != null && devToolsWebContents.equal(webContents)) {
      return window
    }
  }
}

// Helpers.
Object.assign(BrowserWindow.prototype, {
  loadURL (...args) {
    return this.webContents.loadURL(...args)
  },
  getURL (...args) {
    return this.webContents.getURL()
  },
  reload (...args) {
    return this.webContents.reload(...args)
  },
  send (...args) {
    return this.webContents.send(...args)
  },
  openDevTools (...args) {
    return this.webContents.openDevTools(...args)
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
    return this.webContents.inspectElement(...args)
  },
  inspectServiceWorker () {
    return this.webContents.inspectServiceWorker()
  },
  showDefinitionForSelection () {
    return this.webContents.showDefinitionForSelection()
  },
  capturePage (...args) {
    return this.webContents.capturePage(...args)
  },
  setTouchBar (touchBar) {
    electron.TouchBar._setOnWindow(touchBar, this)
  }
})

module.exports = BrowserWindow
