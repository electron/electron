'use strict'

const {EventEmitter} = require('events')
const electron = require('electron')
const {app, ipcMain, session, NavigationController} = electron

// session is not used here, the purpose is to make sure session is initalized
// before the webContents module.
session

let nextId = 0
const getNextId = function () {
  return ++nextId
}

// Stock page sizes
const PDFPageSizes = {
  A5: {
    custom_display_name: 'A5',
    height_microns: 210000,
    name: 'ISO_A5',
    width_microns: 148000
  },
  A4: {
    custom_display_name: 'A4',
    height_microns: 297000,
    name: 'ISO_A4',
    is_default: 'true',
    width_microns: 210000
  },
  A3: {
    custom_display_name: 'A3',
    height_microns: 420000,
    name: 'ISO_A3',
    width_microns: 297000
  },
  Legal: {
    custom_display_name: 'Legal',
    height_microns: 355600,
    name: 'NA_LEGAL',
    width_microns: 215900
  },
  Letter: {
    custom_display_name: 'Letter',
    height_microns: 279400,
    name: 'NA_LETTER',
    width_microns: 215900
  },
  Tabloid: {
    height_microns: 431800,
    name: 'NA_LEDGER',
    width_microns: 279400,
    custom_display_name: 'Tabloid'
  }
}

// Default printing setting
const defaultPrintingSetting = {
  pageRage: [],
  mediaSize: {},
  landscape: false,
  color: 2,
  headerFooterEnabled: false,
  marginsType: 0,
  isFirstRequest: false,
  requestID: getNextId(),
  previewModifiable: true,
  printToPDF: true,
  printWithCloudPrint: false,
  printWithPrivet: false,
  printWithExtension: false,
  deviceName: 'Save as PDF',
  generateDraftData: true,
  fitToPageEnabled: false,
  duplex: 0,
  copies: 1,
  collate: true,
  shouldPrintBackgrounds: false,
  shouldPrintSelectionOnly: false
}

// JavaScript implementations of WebContents.
const binding = process.atomBinding('web_contents')
const {WebContents} = binding

Object.setPrototypeOf(NavigationController.prototype, EventEmitter.prototype)
Object.setPrototypeOf(WebContents.prototype, NavigationController.prototype)

// WebContents::send(channel, args..)
// WebContents::sendToAll(channel, args..)
WebContents.prototype.send = function (channel, ...args) {
  if (channel == null) throw new Error('Missing required channel argument')
  return this._send(false, channel, args)
}
WebContents.prototype.sendToAll = function (channel, ...args) {
  if (channel == null) throw new Error('Missing required channel argument')
  return this._send(true, channel, args)
}

// Following methods are mapped to webFrame.
const webFrameMethods = [
  'insertText',
  'setZoomFactor',
  'setZoomLevel',
  'setZoomLevelLimits'
]
const webFrameMethodsWithResult = [
  'getZoomFactor',
  'getZoomLevel'
]

const asyncWebFrameMethods = function (requestId, method, callback, ...args) {
  this.send('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', requestId, method, args)
  ipcMain.once(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, function (event, result) {
    if (callback) callback(result)
  })
}

const syncWebFrameMethods = function (requestId, method, callback, ...args) {
  this.send('ELECTRON_INTERNAL_RENDERER_SYNC_WEB_FRAME_METHOD', requestId, method, args)
  ipcMain.once(`ELECTRON_INTERNAL_BROWSER_SYNC_WEB_FRAME_RESPONSE_${requestId}`, function (event, result) {
    if (callback) callback(result)
  })
}

for (const method of webFrameMethods) {
  WebContents.prototype[method] = function (...args) {
    this.send('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', method, args)
  }
}

for (const method of webFrameMethodsWithResult) {
  WebContents.prototype[method] = function (...args) {
    const callback = args[args.length - 1]
    const actualArgs = args.slice(0, args.length - 2)
    syncWebFrameMethods.call(this, getNextId(), method, callback, ...actualArgs)
  }
}

// Make sure WebContents::executeJavaScript would run the code only when the
// WebContents has been loaded.
WebContents.prototype.executeJavaScript = function (code, hasUserGesture, callback) {
  const requestId = getNextId()
  if (typeof hasUserGesture === 'function') {
    callback = hasUserGesture
    hasUserGesture = false
  }
  if (this.getURL() && !this.isLoadingMainFrame()) {
    asyncWebFrameMethods.call(this, requestId, 'executeJavaScript', callback, code, hasUserGesture)
  } else {
    this.once('did-finish-load', () => {
      asyncWebFrameMethods.call(this, requestId, 'executeJavaScript', callback, code, hasUserGesture)
    })
  }
}

// Translate the options of printToPDF.
WebContents.prototype.printToPDF = function (options, callback) {
  const printingSetting = Object.assign({}, defaultPrintingSetting)
  if (options.landscape) {
    printingSetting.landscape = options.landscape
  }
  if (options.marginsType) {
    printingSetting.marginsType = options.marginsType
  }
  if (options.printSelectionOnly) {
    printingSetting.shouldPrintSelectionOnly = options.printSelectionOnly
  }
  if (options.printBackground) {
    printingSetting.shouldPrintBackgrounds = options.printBackground
  }

  if (options.pageSize) {
    const pageSize = options.pageSize
    if (typeof pageSize === 'object') {
      if (!pageSize.height || !pageSize.width) {
        return callback(new Error('Must define height and width for pageSize'))
      }
      // Dimensions in Microns
      // 1 meter = 10^6 microns
      printingSetting.mediaSize = {
        name: 'CUSTOM',
        custom_display_name: 'Custom',
        height_microns: pageSize.height,
        width_microns: pageSize.width
      }
    } else if (PDFPageSizes[pageSize]) {
      printingSetting.mediaSize = PDFPageSizes[pageSize]
    } else {
      return callback(new Error(`Does not support pageSize with ${pageSize}`))
    }
  } else {
    printingSetting.mediaSize = PDFPageSizes['A4']
  }

  this._printToPDF(printingSetting, callback)
}

// Add JavaScript wrappers for WebContents class.
WebContents.prototype._init = function () {
  // The navigation controller.
  NavigationController.call(this, this)

  // Every remote callback from renderer process would add a listenter to the
  // render-view-deleted event, so ignore the listenters warning.
  this.setMaxListeners(0)

  // Dispatch IPC messages to the ipc module.
  this.on('ipc-message', function (event, [channel, ...args]) {
    ipcMain.emit(channel, event, ...args)
  })
  this.on('ipc-message-sync', function (event, [channel, ...args]) {
    Object.defineProperty(event, 'returnValue', {
      set: function (value) {
        return event.sendReply(JSON.stringify(value))
      },
      get: function () {}
    })
    ipcMain.emit(channel, event, ...args)
  })

  // Handle context menu action request from pepper plugin.
  this.on('pepper-context-menu', function (event, params) {
    // Access Menu via electron.Menu to prevent circular require
    const menu = electron.Menu.buildFromTemplate(params.menu)
    menu.popup(params.x, params.y)
  })

  // The devtools requests the webContents to reload.
  this.on('devtools-reload-page', function () {
    this.reload()
  })

  // Delays the page-title-updated event to next tick.
  this.on('-page-title-updated', function (...args) {
    setImmediate(() => {
      this.emit.apply(this, ['page-title-updated'].concat(args))
    })
  })

  app.emit('web-contents-created', {}, this)
}

// JavaScript wrapper of Debugger.
const {Debugger} = process.atomBinding('debugger')

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
    for (let contents of binding.getAllWebContents()) {
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
