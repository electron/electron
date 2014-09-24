asar = process.atomBinding 'asar'
fs = require 'fs'
path = require 'path'

# Separate asar package's path from full path.
splitPath = (p) ->
  components = p.split path.sep
  for c, i in components by -1
    if path.extname(c) is '.asar'
      asarPath = components.slice(0, i + 1).join path.sep
      filePath = components.slice(i + 1).join path.sep
      return [true, asarPath, filePath]
  return [false, p]

# Convert asar archive's Stats object to fs's Stats object.
asarStatsToFsStats = (stats) ->
  {
    size: stats.size
    isFile: -> stats.isFile
    isDirectory: -> stats.isDirectory
    isSymbolicLink: -> stats.isLink
    isBlockDevice: -> false
    isCharacterDevice: -> false
    isFIFO: -> false
    isSocket: -> false
  }

# Override fs APIs.
statSync = fs.statSync
fs.statSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return statSync p unless isAsar

  archive = asar.createArchive asarPath
  throw new Error("Invalid package #{asarPath}") unless archive

  stats = archive.stat filePath
  throw new Error("#{filePath} not found in #{asarPath}") unless stats

  asarStatsToFsStats stats

stat = fs.stat
fs.stat = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return stat p, callback unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  stats = asar.createArchive(asarPath).stat filePath
  return callback new Error("#{filePath} not found in #{asarPath}") unless stats

  callback undefined, asarStatsToFsStats stats

statSyncNoException = fs.statSyncNoException
fs.statSyncNoException = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return statSyncNoException p unless isAsar

  archive = asar.createArchive asarPath
  return false unless archive
  stats = asar.createArchive(asarPath).stat filePath
  return false unless stats
  asarStatsToFsStats stats
