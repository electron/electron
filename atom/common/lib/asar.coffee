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

# Create a ENOENT error.
createNotFoundError = (asarPath, filePath) ->
  error = new Error("ENOENT, #{filePath} not found in #{asarPath}")
  error.code = "ENOENT"
  error.errno = -2
  error

# Override fs APIs.
statSync = fs.statSync
fs.statSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return statSync p unless isAsar

  archive = asar.createArchive asarPath
  throw new Error("Invalid package #{asarPath}") unless archive

  stats = archive.stat filePath
  throw createNotFoundError(asarPath, filePath) unless stats

  asarStatsToFsStats stats

stat = fs.stat
fs.stat = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return stat p, callback unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  stats = asar.createArchive(asarPath).stat filePath
  return callback createNotFoundError(asarPath, filePath) unless stats

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

exists = fs.exists
fs.exists = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return exists p, callback unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  callback archive.stat(filePath) isnt false

existsSync = fs.existsSync
fs.existsSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return existsSync p unless isAsar

  archive = asar.createArchive asarPath
  return false unless archive

  archive.stat(filePath) isnt false

readFile = fs.readFile
fs.readFile = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return readFile.apply this, arguments unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  info = archive.getFileInfo filePath
  return callback createNotFoundError(asarPath, filePath) unless info

  buffer = new Buffer(info.size)
  fs.open archive.path, 'r', (error, fd) ->
    return callback error if error
    fs.read fd, buffer, 0, info.size, info.offset, (error) ->
      fs.close fd, ->
        callback error, buffer

readFileSync = fs.readFileSync
fs.readFileSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return readFileSync.apply this, arguments unless isAsar

  archive = asar.createArchive asarPath
  throw new Error("Invalid package #{asarPath}") unless archive

  info = archive.getFileInfo filePath
  throw createNotFoundError(asarPath, filePath) unless info

  buffer = new Buffer(info.size)
  fd = fs.openSync archive.path, 'r'
  fs.readSync fd, buffer, 0, info.size, info.offset
  fs.closeSync fd
  buffer
