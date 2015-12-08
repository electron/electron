{ipcRenderer, NativeImage} = require 'electron'

nextId = 0
getNextId = -> ++nextId

exports.getSources = (options, callback) ->
  id = getNextId()
  ipcRenderer.send 'ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', options, id
  ipcRenderer.once "ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_#{id}", (error_message, sources) ->
    error = if error_message then Error error_message else null
    callback error, ({id: source.id, name: source.name, thumbnail: NativeImage.createFromDataUrl source.thumbnail} for source in sources)
