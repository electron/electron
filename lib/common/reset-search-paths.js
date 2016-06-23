const path = require('path')
const Module = require('module')

// Clear Node's global search paths.
Module.globalPaths.length = 0

// Clear current and parent(init.js)'s search paths.
module.paths = []
module.parent.paths = []

// Prevent Node from adding paths outside this app to search paths.
const originalNodeModulePaths = Module._nodeModulePaths
Module._nodeModulePaths = function (from) {
  const paths = originalNodeModulePaths(from)

  // If "from" is outside the app then we do nothing.
  const rootPath = process.resourcesPath + path.sep
  const fromPath = path.resolve(from) + path.sep
  const skipOutsidePaths = fromPath.startsWith(rootPath)
  if (skipOutsidePaths) {
    return paths.filter(function (candidate) {
      return candidate.startsWith(rootPath)
    })
  }

  return paths
}

// Patch Module._resolveFilename to always require the Electron API when
// require('electron') is done.
const electronPath = path.join(__dirname, '..', process.type, 'api', 'exports', 'electron.js')
const originalResolveFilename = Module._resolveFilename
Module._resolveFilename = function (request, parent, isMain) {
  if (request === 'electron') {
    return electronPath
  } else {
    return originalResolveFilename(request, parent, isMain)
  }
}
