ipc = require 'ipc'
NativeImage = require 'native-image'

getSources = (options, callback) ->
  ipc.send 'ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', options
  ipc.once 'ATOM_RENDERER_DESKTOP_CAPTURER_RESULT', (error_message, sources) ->
    error = if error_message then Error error_message else null
    callback error, ({id: source.id, name: source.name, thumbnail: NativeImage.createFromDataUrl source.thumbnail} for source in sources)

module.exports =
  getSources: getSources
