ipc = require 'ipc'
NativeImage = require 'native-image'

nextId = 0
getNextId = -> ++nextId

getSources = (options, callback) ->
  id = getNextId()
  ipc.send 'ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', options, id
  ipc.once "ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_#{id}", (error_message, sources) ->
    error = if error_message then Error error_message else null
    callback error, ({id: source.id, name: source.name, thumbnail: NativeImage.createFromDataUrl source.thumbnail} for source in sources)

module.exports =
  getSources: getSources
