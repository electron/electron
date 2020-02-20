'use strict'

const features = process.electronBinding('features')
const { EventEmitter } = require('events')
const electron = require('electron')
const path = require('path')
const url = require('url')
const { app, ipcMain, session, deprecate } = electron

const { internalWindowOpen } = require('@electron/internal/browser/guest-window-manager')
const NavigationController = require('@electron/internal/browser/navigation-controller')
const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal')
const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')
const { updatePrintSettings } = require('@electron/internal/browser/printing')

// session is not used here, the purpose is to make sure session is initalized
// before the webContents module.
// eslint-disable-next-line
session

// JavaScript implementations of WebContents.
const binding = process.electronBinding('web_contents')
const { WebContents } = binding

Object.setPrototypeOf(NavigationController.prototype, EventEmitter.prototype)
Object.setPrototypeOf(WebContents.prototype, NavigationController.prototype)

// WebContents::send(channel, args..)
// WebContents::sendToAll(channel, args..)
WebContents.prototype.send = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument')
  }

  const internal = false
  const sendToAll = false

  return this._send(internal, sendToAll, channel, args)
}

WebContents.prototype.sendToAll = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument')
  }

  const internal = false
  const sendToAll = true

  return this._send(internal, sendToAll, channel, args)
}

WebContents.prototype._sendInternal = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument')
  }

  const internal = true
  const sendToAll = false

  return this._send(internal, sendToAll, channel, args)
}
WebContents.prototype._sendInternalToAll = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument')
  }

  const internal = true
  const sendToAll = true

  return this._send(internal, sendToAll, channel, args)
}
WebContents.prototype.sendToFrame = function (frameId, channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument')
  } else if (typeof frameId !== 'number') {
    throw new Error('Missing required frameId argument')
  }

  const internal = false
  const sendToAll = false

  return this._sendToFrame(internal, sendToAll, frameId, channel, args)
}
WebContents.prototype._sendToFrameInternal = function (frameId, channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument')
  } else if (typeof frameId !== 'number') {
    throw new Error('Missing required frameId argument')
  }

  const internal = true
  const sendToAll = false

  return this._sendToFrame(internal, sendToAll, frameId, channel, args)
}

// Following methods are mapped to webFrame.
const webFrameMethods = [
  'insertCSS',
  'insertText',
  'removeInsertedCSS',
  'setVisualZoomLevelLimits'
]

for (const method of webFrameMethods) {
  WebContents.prototype[method] = function (...args) {
    return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', method, ...args)
  }
}

const waitTillCanExecuteJavaScript = async (webContents) => {
  if (webContents.getURL() && !webContents.isLoadingMainFrame()) return

  return new Promise((resolve) => {
    webContents.once('did-stop-loading', () => {
      resolve()
    })
  })
}

// Ensure WebContents::executeJavaScript runs only when the WebContents has been loaded.
WebContents.prototype.executeJavaScript = async function (code, hasUserGesture) {
  await waitTillCanExecuteJavaScript(this)
  return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', 'executeJavaScript', code, hasUserGesture)
}

WebContents.prototype.executeJavaScriptInIsolatedWorld = async function (code, hasUserGesture) {
  await waitTillCanExecuteJavaScript(this)
  return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', 'executeJavaScriptInIsolatedWorld', code, hasUserGesture)
}

WebContents.prototype.printToPDF = function (options = {}) {
  if (features.isPrintingEnabled()) {
    const { error, settings } = updatePrintSettings(options, 'PDF')
    if (error) return Promise.reject(error)

    return this._printToPDF(settings)
  } else {
    const error = new Error('Printing feature is disabled')
    return Promise.reject(error)
  }
}

WebContents.prototype.print = function (options = {}) {
  if (features.isPrintingEnabled()) {
    const { error, settings } = updatePrintSettings(options, 'REGULAR')
    if (error) return Promise.reject(error)

    return this._print(settings)
  } else {
    const error = new Error('Printing feature is disabled')
    return Promise.reject(error)
  }
}

WebContents.prototype.getPrinters = function () {
  if (features.isPrintingEnabled()) {
    return this._getPrinters()
  } else {
    console.error('Error: Printing feature is disabled.')
    return []
  }
}

WebContents.prototype.loadFile = function (filePath, options = {}) {
  if (typeof filePath !== 'string') {
    throw new Error('Must pass filePath as a string')
  }
  const { query, search, hash } = options

  return this.loadURL(url.format({
    protocol: 'file',
    slashes: true,
    pathname: path.resolve(app.getAppPath(), filePath),
    query,
    search,
    hash
  }))
}

const addReplyToEvent = (event) => {
  event.reply = (...args) => {
    event.sender.sendToFrame(event.frameId, ...args)
  }
}

const addReplyInternalToEvent = (event) => {
  Object.defineProperty(event, '_replyInternal', {
    configurable: false,
    enumerable: false,
    value: (...args) => {
      event.sender._sendToFrameInternal(event.frameId, ...args)
    }
  })
}

const addReturnValueToEvent = (event) => {
  Object.defineProperty(event, 'returnValue', {
    set: (value) => event.sendReply([value]),
    get: () => {}
  })
}

// Add JavaScript wrappers for WebContents class.
WebContents.prototype._init = function () {
  // The navigation controller.
  NavigationController.call(this, this)

  // Every remote callback from renderer process would add a listener to the
  // render-view-deleted event, so ignore the listeners warning.
  this.setMaxListeners(0)

  // Dispatch IPC messages to the ipc module.
  this.on('-ipc-message', function (event, internal, channel, args) {
    if (internal) {
      addReplyInternalToEvent(event)
      ipcMainInternal.emit(channel, event, ...args)
    } else {
      addReplyToEvent(event)
      this.emit('ipc-message', event, channel, ...args)
      ipcMain.emit(channel, event, ...args)
    }
  })

  this.on('-ipc-invoke', function (event, internal, channel, args) {
    event._reply = (result) => event.sendReply({ result })
    event._throw = (error) => {
      console.error(`Error occurred in handler for '${channel}':`, error)
      event.sendReply({ error: error.toString() })
    }
    const target = internal ? ipcMainInternal : ipcMain
    if (target._invokeHandlers.has(channel)) {
      target._invokeHandlers.get(channel)(event, ...args)
    } else {
      event._throw(`No handler registered for '${channel}'`)
    }
  })

  this.on('-ipc-message-sync', function (event, internal, channel, args) {
    addReturnValueToEvent(event)
    if (internal) {
      addReplyInternalToEvent(event)
      ipcMainInternal.emit(channel, event, ...args)
    } else {
      addReplyToEvent(event)
      this.emit('ipc-message-sync', event, channel, ...args)
      ipcMain.emit(channel, event, ...args)
    }
  })

  // Handle context menu action request from pepper plugin.
  this.on('pepper-context-menu', function (event, params, callback) {
    // Access Menu via electron.Menu to prevent circular require.
    const menu = electron.Menu.buildFromTemplate(params.menu)
    menu.popup({
      window: event.sender.getOwnerBrowserWindow(),
      x: params.x,
      y: params.y,
      callback
    })
  })

  this.on('crashed', (event, ...args) => {
    app.emit('renderer-process-crashed', event, this, ...args)
  })

  // The devtools requests the webContents to reload.
  this.on('devtools-reload-page', function () {
    this.reload()
  })

  // Handle window.open for BrowserWindow and BrowserView.
  if (['browserView', 'window'].includes(this.getType())) {
    // Make new windows requested by links behave like "window.open".
    this.on('-new-window', (event, url, frameName, disposition,
      additionalFeatures, postData,
      referrer) => {
      const options = {
        show: true,
        width: 800,
        height: 600
      }
      internalWindowOpen(event, url, referrer, frameName, disposition, options, additionalFeatures, postData)
    })

    // Create a new browser window for the native implementation of
    // "window.open", used in sandbox and nativeWindowOpen mode.
    this.on('-add-new-contents', (event, webContents, disposition,
      userGesture, left, top, width, height, url, frameName) => {
      if ((disposition !== 'foreground-tab' && disposition !== 'new-window' &&
           disposition !== 'background-tab')) {
        event.preventDefault()
        return
      }

      const options = {
        show: true,
        x: left,
        y: top,
        width: width || 800,
        height: height || 600,
        webContents
      }
      const referrer = { url: '', policy: 'default' }
      internalWindowOpen(event, url, referrer, frameName, disposition, options)
    })
  }

  this.on('login', (event, ...args) => {
    app.emit('login', event, this, ...args)
  })

  const event = process.electronBinding('event').createEmpty()
  app.emit('web-contents-created', event, this)
}

// Deprecations
deprecate.fnToProperty(WebContents.prototype, 'audioMuted', '_isAudioMuted', '_setAudioMuted')
deprecate.fnToProperty(WebContents.prototype, 'userAgent', '_getUserAgent', '_setUserAgent')
deprecate.fnToProperty(WebContents.prototype, 'zoomLevel', '_getZoomLevel', '_setZoomLevel')
deprecate.fnToProperty(WebContents.prototype, 'zoomFactor', '_getZoomFactor', '_setZoomFactor')
deprecate.fnToProperty(WebContents.prototype, 'frameRate', '_getFrameRate', '_setFrameRate')

// JavaScript wrapper of Debugger.
const { Debugger } = process.electronBinding('debugger')
Object.setPrototypeOf(Debugger.prototype, EventEmitter.prototype)

// Public APIs.
module.exports = {
  create (options = {}) {
    return binding.create(options)
  },

  fromId (id) {
    return binding.fromId(id)
  },

  getFocusedWebContents () {
    let focused = null
    for (const contents of binding.getAllWebContents()) {
      if (!contents.isFocused()) continue
      if (focused == null) focused = contents
      // Return webview web contents which may be embedded inside another
      // web contents that is also reporting as focused
      if (contents.getType() === 'webview') return contents
    }
    return focused
  },

  getAllWebContents () {
    return binding.getAllWebContents()
  }
}
