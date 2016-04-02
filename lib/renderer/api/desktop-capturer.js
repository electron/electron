const ipcRenderer = require('electron').ipcRenderer
const nativeImage = require('electron').nativeImage

var nextId = 0
var includes = [].includes

var getNextId = function () {
  return ++nextId
}

// |options.type| can not be empty and has to include 'window' or 'screen'.
var isValid = function (options) {
  return ((options != null ? options.types : void 0) != null) && Array.isArray(options.types)
}

exports.getSources = function (options, callback) {
  var captureScreen, captureWindow, id
  if (!isValid(options)) {
    return callback(new Error('Invalid options'))
  }
  captureWindow = includes.call(options.types, 'window')
  captureScreen = includes.call(options.types, 'screen')
  if (options.thumbnailSize == null) {
    options.thumbnailSize = {
      width: 150,
      height: 150
    }
  }
  id = getNextId()
  ipcRenderer.send('ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', captureWindow, captureScreen, options.thumbnailSize, id)
  return ipcRenderer.once('ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_' + id, function (event, sources) {
    var source
    return callback(null, (function () {
      var i, len, results
      results = []
      for (i = 0, len = sources.length; i < len; i++) {
        source = sources[i]
        results.push({
          id: source.id,
          name: source.name,
          thumbnail: nativeImage.createFromDataURL(source.thumbnail)
        })
      }
      return results
    })())
  })
}
