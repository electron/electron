'use strict'

const {EventEmitter} = require('events')
const {ipcMain, Menu, NavigationController} = require('electron')

const binding = process.atomBinding('web_contents')
const debuggerBinding = process.atomBinding('debugger')
const sessionBinding = process.atomBinding('session')

let nextId = 0

let getNextId = function () {
  return ++nextId
}

let PDFPageSize = {
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

// Following methods are mapped to webFrame.
const webFrameMethods = [
  'insertText',
  'setZoomFactor',
  'setZoomLevel',
  'setZoomLevelLimits'
]

let wrapWebContents = function (webContents) {
  // webContents is an EventEmitter.
  var controller, method, name, ref1
  Object.setPrototypeOf(webContents, EventEmitter.prototype)

  // Every remote callback from renderer process would add a listenter to the
  // render-view-deleted event, so ignore the listenters warning.
  webContents.setMaxListeners(0)

  // WebContents::send(channel, args..)
  // WebContents::sendToAll(channel, args..)
  const sendWrapper = (allFrames, channel, ...args) => {
    if (channel == null) {
      throw new Error('Missing required channel argument')
    }
    return webContents._send(allFrames, channel, args)
  }
  webContents.send = sendWrapper.bind(null, false)
  webContents.sendToAll = sendWrapper.bind(null, true)

  // The navigation controller.
  controller = new NavigationController(webContents)
  ref1 = NavigationController.prototype
  for (name in ref1) {
    method = ref1[name]
    if (method instanceof Function) {
      (function (name, method) {
        webContents[name] = function () {
          return method.apply(controller, arguments)
        }
      })(name, method)
    }
  }

  // Mapping webFrame methods.
  for (let method of webFrameMethods) {
    webContents[method] = function (...args) {
      this.send('ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', method, args)
    }
  }

  const asyncWebFrameMethods = function (requestId, method, callback, ...args) {
    this.send('ELECTRON_INTERNAL_RENDERER_ASYNC_WEB_FRAME_METHOD', requestId, method, args)
    ipcMain.once(`ELECTRON_INTERNAL_BROWSER_ASYNC_WEB_FRAME_RESPONSE_${requestId}`, function (event, result) {
      if (callback) callback(result)
    })
  }

  // Make sure webContents.executeJavaScript would run the code only when the
  // webContents has been loaded.
  webContents.executeJavaScript = function (code, hasUserGesture, callback) {
    let requestId = getNextId()
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

  // Dispatch IPC messages to the ipc module.
  webContents.on('ipc-message', function (event, [channel, ...args]) {
    ipcMain.emit.apply(ipcMain, [channel, event].concat(args))
  })
  webContents.on('ipc-message-sync', function (event, [channel, ...args]) {
    Object.defineProperty(event, 'returnValue', {
      set: function (value) {
        return event.sendReply(JSON.stringify(value))
      },
      get: function () {
        return undefined
      }
    })
    return ipcMain.emit.apply(ipcMain, [channel, event].concat(args))
  })

  // Handle context menu action request from pepper plugin.
  webContents.on('pepper-context-menu', function (event, params) {
    const menu = Menu.buildFromTemplate(params.menu)
    menu.popup(params.x, params.y)
  })

  // The devtools requests the webContents to reload.
  webContents.on('devtools-reload-page', function () {
    webContents.reload()
  })

  // Delays the page-title-updated event to next tick.
  webContents.on('-page-title-updated', function (...args) {
    setImmediate(() => {
      this.emit.apply(this, ['page-title-updated'].concat(args))
    })
  })

  webContents.printToPDF = function (options, callback) {
    var printingSetting
    printingSetting = {
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
    if (options.pageSize && PDFPageSize[options.pageSize]) {
      printingSetting.mediaSize = PDFPageSize[options.pageSize]
    } else {
      printingSetting.mediaSize = PDFPageSize['A4']
    }
    return this._printToPDF(printingSetting, callback)
  }
}

// Wrapper for native class.
let wrapDebugger = function (webContentsDebugger) {
  // debugger is an EventEmitter.
  Object.setPrototypeOf(webContentsDebugger, EventEmitter.prototype)
}

var wrapSession = function (session) {
  // session is an EventEmitter.
  Object.setPrototypeOf(session, EventEmitter.prototype)
}

binding._setWrapWebContents(wrapWebContents)
debuggerBinding._setWrapDebugger(wrapDebugger)
sessionBinding._setWrapSession(wrapSession)

module.exports = {
  create (options = {}) {
    return binding.create(options)
  },

  fromId (id) {
    return binding.fromId(id)
  }
}
