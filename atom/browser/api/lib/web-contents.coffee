{EventEmitter} = require 'events'
{deprecate, ipcMain, session, NavigationController, Menu} = require 'electron'

binding = process.atomBinding 'web_contents'

nextId = 0
getNextId = -> ++nextId

PDFPageSize =
  A4:
    custom_display_name: "A4"
    height_microns: 297000
    name: "ISO_A4"
    is_default: "true"
    width_microns: 210000
  A3:
    custom_display_name: "A3"
    height_microns: 420000
    name: "ISO_A3"
    width_microns: 297000
  Legal:
    custom_display_name: "Legal"
    height_microns: 355600
    name: "NA_LEGAL"
    width_microns: 215900
  Letter:
    custom_display_name: "Letter"
    height_microns: 279400
    name: "NA_LETTER"
    width_microns: 215900
  Tabloid:
    height_microns: 431800
    name: "NA_LEDGER"
    width_microns: 279400
    custom_display_name: "Tabloid"

wrapWebContents = (webContents) ->
  # webContents is an EventEmitter.
  webContents.__proto__ = EventEmitter.prototype

  # WebContents::send(channel, args..)
  webContents.send = (channel, args...) ->
    @_send channel, [args...]

  # Make sure webContents.executeJavaScript would run the code only when the
  # web contents has been loaded.
  webContents.executeJavaScript = (code, hasUserGesture=false) ->
    if @getURL() and not @isLoading()
      @_executeJavaScript code, hasUserGesture
    else
      webContents.once 'did-finish-load', @_executeJavaScript.bind(this, code, hasUserGesture)

  # The navigation controller.
  controller = new NavigationController(webContents)
  for name, method of NavigationController.prototype when method instanceof Function
    do (name, method) ->
      webContents[name] = -> method.apply controller, arguments

  # Dispatch IPC messages to the ipc module.
  webContents.on 'ipc-message', (event, packed) ->
    [channel, args...] = packed
    ipcMain.emit channel, event, args...
  webContents.on 'ipc-message-sync', (event, packed) ->
    [channel, args...] = packed
    Object.defineProperty event, 'returnValue', set: (value) -> event.sendReply JSON.stringify(value)
    ipcMain.emit channel, event, args...

  # Handle context menu action request from pepper plugin.
  webContents.on 'pepper-context-menu', (event, params) ->
    menu = Menu.buildFromTemplate params.menu
    menu.popup params.x, params.y

  # This error occurs when host could not be found.
  webContents.on 'did-fail-provisional-load', (args...) ->
    # Calling loadURL during this event might cause crash, so delay the event
    # until next tick.
    setImmediate => @emit 'did-fail-load', args...

  # Delays the page-title-set event to next tick.
  webContents.on '-page-title-set', (args...) ->
    setImmediate => @emit 'page-title-set', args...

  # Deprecated.
  deprecate.rename webContents, 'loadUrl', 'loadURL'
  deprecate.rename webContents, 'getUrl', 'getURL'

  webContents.printToPDF = (options, callback) ->
    printingSetting =
      pageRage: []
      mediaSize: {}
      landscape: false
      color: 2
      headerFooterEnabled: false
      marginsType: 0
      isFirstRequest: false
      requestID: getNextId()
      previewModifiable: true
      printToPDF: true
      printWithCloudPrint: false
      printWithPrivet: false
      printWithExtension: false
      deviceName: "Save as PDF"
      generateDraftData: true
      fitToPageEnabled: false
      duplex: 0
      copies: 1
      collate: true
      shouldPrintBackgrounds: false
      shouldPrintSelectionOnly: false

    if options.landscape
      printingSetting.landscape = options.landscape
    if options.marginsType
      printingSetting.marginsType = options.marginsType
    if options.printSelectionOnly
      printingSetting.shouldPrintSelectionOnly = options.printSelectionOnly
    if options.printBackground
      printingSetting.shouldPrintBackgrounds = options.printBackground

    if options.pageSize and PDFPageSize[options.pageSize]
      printingSetting.mediaSize = PDFPageSize[options.pageSize]
    else
      printingSetting.mediaSize = PDFPageSize['A4']

    @_printToPDF printingSetting, callback

binding._setWrapWebContents wrapWebContents

module.exports.create = (options={}) ->
  binding.create(options)
