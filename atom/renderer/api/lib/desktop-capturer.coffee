{ipcRenderer, nativeImage} = require 'electron'

nextId = 0
getNextId = -> ++nextId

# |options.type| can not be empty and has to include 'window' or 'screen'.
isValid = (options) ->
  return options?.types? and Array.isArray options.types

exports.getSources = (options, callback) ->
  return callback new Error('Invalid options') unless isValid options

  captureWindow = 'window' in options.types
  captureScreen = 'screen' in options.types
  options.thumbnailSize ?= width: 150, height: 150

  id = getNextId()
  ipcRenderer.send 'ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', captureWindow, captureScreen, options.thumbnailSize, id
  ipcRenderer.once "ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_#{id}", (event, sources) ->
    callback null, ({id: source.id, name: source.name, thumbnail: nativeImage.createFromDataURL source.thumbnail} for source in sources)
