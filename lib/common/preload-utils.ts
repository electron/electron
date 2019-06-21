import * as fs from 'fs'
import * as path from 'path'

const isParentDir = function (parent: string, dir: string) {
  const relative = path.relative(parent, dir)
  return !!relative && !relative.startsWith('..') && !path.isAbsolute(relative)
}

const isPreloadAllowedImpl = function (appPath: string, preloadPath: string) {
  const absoluteResourcesPath = process.resourcesPath ? fs.realpathSync(process.resourcesPath) : null

  // only allow loading preloads from resources when app is bundled
  if (absoluteResourcesPath && isParentDir(absoluteResourcesPath, fs.realpathSync(appPath))) {
    return isParentDir(absoluteResourcesPath, fs.realpathSync(preloadPath))
  }

  return true
}

const cache = new Map<string, boolean>()

export function isPreloadAllowed (appPath: string, preloadPath: string) {
  if (!cache.has(preloadPath)) {
    cache.set(preloadPath, isPreloadAllowedImpl(appPath, preloadPath))
  }

  return cache.get(preloadPath)
}
