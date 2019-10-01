import { nativeImage } from 'electron'
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'

// |options.types| can't be empty and must be an array
function isValid (options: Electron.SourcesOptions) {
  const types = options ? options.types : undefined
  return Array.isArray(types)
}

export async function getSources (options: Electron.SourcesOptions) {
  if (!isValid(options)) throw new Error('Invalid options')

  const captureWindow = options.types.includes('window')
  const captureScreen = options.types.includes('screen')

  const { thumbnailSize = { width: 150, height: 150 } } = options
  const { fetchWindowIcons = false } = options

  const sources = await ipcRendererInternal.invoke<ElectronInternal.GetSourcesResult[]>('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', {
    captureWindow,
    captureScreen,
    thumbnailSize,
    fetchWindowIcons
  } as ElectronInternal.GetSourcesOptions)

  return sources.map(source => ({
    id: source.id,
    name: source.name,
    thumbnail: nativeImage.createFromDataURL(source.thumbnail),
    display_id: source.display_id,
    appIcon: source.appIcon ? nativeImage.createFromDataURL(source.appIcon) : null
  }))
}
