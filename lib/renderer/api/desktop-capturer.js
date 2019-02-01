'use strict'

const { nativeImage, deprecate } = require('electron')
const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')

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

function mapSources (sources) {
  return sources.map(source => ({
    id: source.id,
    name: source.name,
    thumbnail: nativeImage.createFromDataURL(source.thumbnail),
    display_id: source.display_id,
    appIcon: source.appIcon ? nativeImage.createFromDataURL(source.appIcon) : null
  }))
}

const getSources = (options) => {
  return new Promise((resolve, reject) => {
    if (!isValid(options)) throw new Error('Invalid options')

    const captureWindow = includes.call(options.types, 'window')
    const captureScreen = includes.call(options.types, 'screen')

    if (options.thumbnailSize == null) {
      options.thumbnailSize = {
        width: 150,
        height: 150
      }
    }
    if (options.fetchWindowIcons == null) {
      options.fetchWindowIcons = false
    }

    const id = incrementId()
    ipcRenderer.send('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', captureWindow, captureScreen, options.thumbnailSize, options.fetchWindowIcons, id)
    return ipcRenderer.once(`ELECTRON_RENDERER_DESKTOP_CAPTURER_RESULT_${id}`, (event, sources) => {
      try {
        resolve(mapSources(sources))
      } catch (error) {
        reject(error)
      }
    })
  })
}

exports.getSources = deprecate.promisify(getSources)
