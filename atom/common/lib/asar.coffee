asar = process.atomBinding 'asar'
fs = require 'fs'
path = require 'path'
util = require 'util'

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
nextInode = 0
uid = if process.getuid? then process.getuid() else 0
gid = if process.getgid? then process.getgid() else 0
asarStatsToFsStats = (stats) ->
  {
    dev: 1,
    ino: ++nextInode,
    mode: 33188,
    nlink: 1,
    uid: uid,
    gid: gid,
    rdev: 0,
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
lstatSync = fs.lstatSync
fs.lstatSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return lstatSync p unless isAsar

  archive = asar.createArchive asarPath
  throw new Error("Invalid package #{asarPath}") unless archive

  stats = archive.stat filePath
  throw createNotFoundError(asarPath, filePath) unless stats

  asarStatsToFsStats stats

lstat = fs.lstat
fs.lstat = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return lstat p, callback unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  stats = asar.createArchive(asarPath).stat filePath
  return callback createNotFoundError(asarPath, filePath) unless stats

  callback undefined, asarStatsToFsStats stats

statSync = fs.statSync
fs.statSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return statSync p unless isAsar

  # Do not distinguish links for now.
  fs.lstatSync p

stat = fs.stat
fs.stat = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return stat p, callback unless isAsar

  # Do not distinguish links for now.
  fs.lstat p, callback

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
fs.readFile = (p, options, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return readFile.apply this, arguments unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  info = archive.getFileInfo filePath
  return callback createNotFoundError(asarPath, filePath) unless info

  if typeof options is 'function'
    callback = options
    options = undefined

  if not options
    options = encoding: null, flag: 'r'
  else if util.isString options
    options = encoding: options, flag: 'r'
  else if not util.isObject options
    throw new TypeError('Bad arguments')

  flag = options.flag || 'r'
  encoding = options.encoding

  buffer = new Buffer(info.size)
  fs.open archive.path, flag, (error, fd) ->
    return callback error if error
    fs.read fd, buffer, 0, info.size, info.offset, (error) ->
      fs.close fd, ->
        callback error, if encoding then buffer.toString encoding else buffer

readFileSync = fs.readFileSync
fs.readFileSync = (p, options) ->
  [isAsar, asarPath, filePath] = splitPath p
  return readFileSync.apply this, arguments unless isAsar

  archive = asar.createArchive asarPath
  throw new Error("Invalid package #{asarPath}") unless archive

  info = archive.getFileInfo filePath
  throw createNotFoundError(asarPath, filePath) unless info

  if not options
    options = encoding: null, flag: 'r'
  else if util.isString options
    options = encoding: options, flag: 'r'
  else if not util.isObject options
    throw new TypeError('Bad arguments')

  flag = options.flag || 'r'
  encoding = options.encoding

  buffer = new Buffer(info.size)
  fd = fs.openSync archive.path, flag
  try
    fs.readSync fd, buffer, 0, info.size, info.offset
  catch e
    throw e
  finally
    fs.closeSync fd
  if encoding then buffer.toString encoding else buffer

readdir = fs.readdir
fs.readdir = (p, callback) ->
  [isAsar, asarPath, filePath] = splitPath p
  return readdir.apply this, arguments unless isAsar

  archive = asar.createArchive asarPath
  return callback throw new Error("Invalid package #{asarPath}") unless archive

  files = archive.readdir filePath
  return callback createNotFoundError(asarPath, filePath) unless files

  process.nextTick ->
    callback undefined, files

readdirSync = fs.readdirSync
fs.readdirSync = (p) ->
  [isAsar, asarPath, filePath] = splitPath p
  return readdirSync.apply this, arguments unless isAsar

  archive = asar.createArchive asarPath
  throw new Error("Invalid package #{asarPath}") unless archive

  files = archive.readdir filePath
  throw createNotFoundError(asarPath, filePath) unless files

  files
