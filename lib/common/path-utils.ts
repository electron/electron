import * as path from 'path'
const Module = require('module')

const isParentDir = function (parent: string, dir: string) {
  const relative = path.relative(parent, dir)
  return !!relative && !relative.startsWith('..') && !path.isAbsolute(relative)
}

const isPreloadAllowedImpl = function (appPath: string, preloadPath: string) {
  if (isParentDir(appPath, preloadPath)) {
    return true
  }

  // allow node_modules to use their preload scripts
  const nodeModulePaths = Module._nodeModulePaths(appPath) as string[]
  return nodeModulePaths.some(nodeModulePath => isParentDir(nodeModulePath, preloadPath))
}

const cache = new Map<string, boolean>()

export const isPreloadAllowed = function (appPath: string, preloadPath: string) {
  if (!cache.has(preloadPath)) {
    cache.set(preloadPath, isPreloadAllowedImpl(appPath, preloadPath))
  }

  return cache.get(preloadPath)
}
