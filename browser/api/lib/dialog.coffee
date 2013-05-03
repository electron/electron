EventEmitter = require('events').EventEmitter
ipc = require 'ipc'

FileDialog = process.atomBinding('file_dialog').FileDialog
FileDialog.prototype.__proto__ = EventEmitter.prototype

callbacksInfo = {}

fileDialog = new FileDialog

onSelected = (event, id, paths...) ->
  {processId, routingId} = callbacksInfo[id]
  delete callbacksInfo[id]

  ipc.sendChannel processId, routingId, 'ATOM_RENDERER_DIALOG', id, paths...

fileDialog.on 'selected', onSelected
fileDialog.on 'cancelled', onSelected

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

ipc.on 'ATOM_BROWSER_FILE_DIALOG', (processId, routingId, callbackId, type, title, options) ->
  options = {} unless options?
  options.type = type
  options.title = title unless options.title?

  throw new TypeError('Bad arguments') unless validateOptions options

  callbacksInfo[callbackId] = processId: processId, routingId: routingId

  fileDialog.selectFile processId, routingId,
                        options.type,
                        options.title,
                        options.defaultPath,
                        options.fileTypes,
                        options.fileTypeIndex,
                        options.defaultExtension,
                        callbackId
