const {ipcRenderer, nativeImage} = require('electron')

const includes = [].includes
let currentId = 0

const incrementId = () => {
  currentId += 1
  return currentId
}

// |options.types| can't be empty and must be an array
function isValid (options) {
  const types = options ? options.types : undefined
  return Array.isArray(types)
}

exports.getSources = function (options, callback) {
  if (!isValid(options)) return callback(new Error('Invalid options'))
  const captureWindow = includes.call(options.types, 'window')
  const captureScreen = includes.call(options.types, 'screen')

  if (options.thumbnailSize == null) {
    options.thumbnailSize = {
      width: 150,
      height: 150
    }
  }

  const id = incrementId()
  ipcRenderer.send('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', captureWindow, captureScreen, options.thumbnailSize, id)
  return ipcRenderer.once(`ELECTRON_RENDERER_DESKTOP_CAPTURER_RESULT_${id}`, (event, sources) => {
    callback(null, (() => {
      const results = []
      sources.forEach(source => {
        results.push({
          id: source.id,
          name: source.name,
          thumbnail: nativeImage.createFromDataURL(source.thumbnail),
          display_id: source.display_id
        })
      })
      return results
    })())
  })
}
