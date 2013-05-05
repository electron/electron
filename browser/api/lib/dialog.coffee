binding = process.atomBinding 'dialog'
EventEmitter = require('events').EventEmitter
ipc = require 'ipc'

FileDialog = binding.FileDialog
FileDialog.prototype.__proto__ = EventEmitter.prototype

class CallbacksRegistry
  @nextId = 0
  @callbacks = {}

  @add: (callback) ->
    @callbacks[++@nextId] = callback
    @nextId

  @get: (id) ->
    @callbacks[id]

  @call: (id, args...) ->
    @callbacks[id].call global, args...

  @remove: (id) ->
    delete @callbacks[id]

fileDialog = new FileDialog

fileDialog.on 'selected', (event, callbackId, paths...) ->
  CallbacksRegistry.call callbackId, 'selected', paths...
  CallbacksRegistry.remove callbackId

fileDialog.on 'cancelled', (event, callbackId) ->
  CallbacksRegistry.call callbackId, 'cancelled'
  CallbacksRegistry.remove callbackId

validateOptions = (options) ->
  return false unless typeof options is 'object'

  options.fileTypes = [] unless Array.isArray options.fileTypes
  for type in options.fileTypes
    return false unless typeof type is 'object' and
                        typeof type.description is 'string'
                        Array.isArray type.extensions

  options.defaultPath = '' unless options.defaultPath?
  options.fileTypeIndex = 0 unless options.fileTypeIndex?
  options.defaultExtension = '' unless options.defaultExtension?
  true

selectFileWrap = (window, options, callback, type, title) ->
  throw new TypeError('Need Window object') unless window.constructor?.name is 'Window'

  options = {} unless options?
  options.type = type
  options.title = title unless options.title?

  throw new TypeError('Bad arguments') unless validateOptions options

  callbackId = CallbacksRegistry.add callback

  fileDialog.selectFile window.getProcessId(), window.getRoutingId(),
                        options.type,
                        options.title,
                        options.defaultPath,
                        options.fileTypes,
                        options.fileTypeIndex,
                        options.defaultExtension,
                        callbackId

module.exports =
  openFolder: (args...) ->
    selectFileWrap args..., 1, 'Open Folder'

  saveAs: (args...) ->
    selectFileWrap args..., 2, 'Save As'

  openFile: (args...) ->
    selectFileWrap args..., 3, 'Open File'

  openMultiFiles: (args...) ->
    selectFileWrap args..., 4, 'Open Files'
