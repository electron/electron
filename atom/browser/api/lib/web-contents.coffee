EventEmitter = require('events').EventEmitter
Menu = require './menu'
NavigationController = require './navigation-controller'
binding = process.atomBinding 'web_contents'
ipc = require 'ipc'

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

clickHandler = (action) ->
  @_executeContextMenuCommand action

convertToMenuTemplate = (items, handler) ->
  template = []
  for item in items
    do (item) ->
      transformed =
        if item.type is 'submenu'
          type: 'submenu'
          label: item.label
          enabled: item.enabled
          submenu: convertToMenuTemplate item.subItems, handler
        else if item.type is 'separator'
          type: 'separator'
        else if item.type is 'checkbox'
          type: 'checkbox'
          label: item.label
          enabled: item.enabled
          checked: item.checked
        else
          type: 'normal'
          label: item.label
          enabled: item.enabled
      if item.id?
        transformed.click = ->
          handler item.id
      template.push transformed
  template

wrapWebContents = (webContents) ->
  # webContents is an EventEmitter.
  webContents.__proto__ = EventEmitter.prototype

  # WebContents::send(channel, args..)
  webContents.send = (channel, args...) ->
    @_send channel, [args...]

  # Make sure webContents.executeJavaScript would run the code only when the
  # web contents has been loaded.
  webContents.executeJavaScript = (code, hasUserGesture=false) ->
    if @getUrl() and not @isLoading()
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
    ipc.emit channel, event, args...
  webContents.on 'ipc-message-sync', (event, packed) ->
    [channel, args...] = packed
    Object.defineProperty event, 'returnValue', set: (value) -> event.sendReply JSON.stringify(value)
    ipc.emit channel, event, args...

  # Handle context menu action request from renderer widget.
  webContents.on '-context-menu', (event, params) ->
    if params.isPepperMenu
      template = convertToMenuTemplate(params.menuItems, clickHandler.bind(webContents))
      menu = Menu.buildFromTemplate template
      # The menu is expected to show asynchronously.
      setImmediate ->
        menu.popup params.x, params.y
        webContents._notifyContextMenuClosed()

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
process.once 'exit', binding._clearWrapWebContents

module.exports.create = (options={}) ->
  binding.create(options)
