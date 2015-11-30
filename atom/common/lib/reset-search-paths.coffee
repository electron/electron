path   = require 'path'
Module = require 'module'

# Clear Node's global search paths.
Module.globalPaths.length = 0

# Clear current and parent(init.coffee)'s search paths.
module.paths = []
module.parent.paths = []

# Prevent Node from adding paths outside this app to search paths.
Module._nodeModulePaths = (from) ->
  from = path.resolve from

  # If "from" is outside the app then we do nothing.
  skipOutsidePaths = from.startsWith process.resourcesPath

  # Following logoic is copied from module.js.
  splitRe = if process.platform is 'win32' then /[\/\\]/ else /\//
  paths = []

  parts = from.split splitRe
  for part, tip in parts by -1
    continue if part is 'node_modules'
    dir = parts.slice(0, tip + 1).join path.sep
    break if skipOutsidePaths and not dir.startsWith process.resourcesPath
    paths.push path.join(dir, 'node_modules')

  paths
