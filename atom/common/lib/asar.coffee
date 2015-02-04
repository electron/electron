asar = process.binding 'atom_common_asar'
child_process = require 'child_process'
path = require 'path'
util = require 'util'

# Cache asar archive objects.
cachedArchives = {}
getOrCreateArchive = (p) ->
  archive = cachedArchives[p]
  return archive if archive?
  archive = asar.createArchive p
  return false unless archive
  cachedArchives[p] = archive

# Clean cache on quit.
process.on 'exit', ->
  archive.destroy() for p, archive of cachedArchives

# Separate asar package's path from full path.
splitPath = (p) ->
  return [false] if typeof p isnt 'string'
  return [true, p, ''] if p.substr(-5) is '.asar'
  index = p.lastIndexOf ".asar#{path.sep}"
  return [false] if index is -1
  [true, p.substr(0, index + 5), p.substr(index + 6)]

# Convert asar archive's Stats object to fs's Stats object.
nextInode = 0
uid = if process.getuid? then process.getuid() else 0
gid = if process.getgid? then process.getgid() else 0
fakeTime = new Date()
asarStatsToFsStats = (stats) ->
  {
    dev: 1,
    ino: ++nextInode,
    mode: 33188,
    nlink: 1,
    uid: uid,
    gid: gid,
    rdev: 0,
    atime: stats.atime || fakeTime,
    birthtime: stats.birthtime || fakeTime,
    mtime: stats.mtime || fakeTime,
    ctime: stats.ctime || fakeTime,
    size: stats.size,
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

# Override APIs that rely on passing file path instead of content to C++.
overrideAPISync = (module, name, arg = 0) ->
  old = module[name]
  module[name] = ->
    p = arguments[arg]
    [isAsar, asarPath, filePath] = splitPath p
    return old.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    throw new Error("Invalid package #{asarPath}") unless archive

    newPath = archive.copyFileOut filePath
    throw createNotFoundError(asarPath, filePath) unless newPath

    arguments[arg] = newPath
    old.apply this, arguments

overrideAPI = (module, name, arg = 0) ->
  old = module[name]
  module[name] = ->
    p = arguments[arg]
    [isAsar, asarPath, filePath] = splitPath p
    return old.apply this, arguments unless isAsar

    callback = arguments[arguments.length - 1]
    return overrideAPISync module, name, arg unless typeof callback is 'function'

    archive = getOrCreateArchive asarPath
    return callback new Error("Invalid package #{asarPath}") unless archive

    newPath = archive.copyFileOut filePath
    return callback  createNotFoundError(asarPath, filePath) unless newPath

    arguments[arg] = newPath
    old.apply this, arguments

# Override fs APIs.
exports.wrapFsWithAsar = (fs) ->
  lstatSync = fs.lstatSync
  fs.lstatSync = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return lstatSync p unless isAsar

    archive = getOrCreateArchive asarPath
    throw new Error("Invalid package #{asarPath}") unless archive

    stats = archive.stat filePath
    throw createNotFoundError(asarPath, filePath) unless stats

    asarStatsToFsStats stats

  lstat = fs.lstat
  fs.lstat = (p, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return lstat p, callback unless isAsar

    archive = getOrCreateArchive asarPath
    return callback new Error("Invalid package #{asarPath}") unless archive

    stats = getOrCreateArchive(asarPath).stat filePath
    return callback createNotFoundError(asarPath, filePath) unless stats

    process.nextTick -> callback null, asarStatsToFsStats stats

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
    process.nextTick -> fs.lstat p, callback

  statSyncNoException = fs.statSyncNoException
  fs.statSyncNoException = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return statSyncNoException p unless isAsar

    archive = getOrCreateArchive asarPath
    return false unless archive
    stats = archive.stat filePath
    return false unless stats
    asarStatsToFsStats stats

  realpathSync = fs.realpathSync
  fs.realpathSync = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return realpathSync.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    throw new Error("Invalid package #{asarPath}") unless archive

    real = archive.realpath filePath
    throw createNotFoundError(asarPath, filePath) if real is false

    path.join realpathSync(asarPath), real

  realpath = fs.realpath
  fs.realpath = (p, cache, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return realpath.apply this, arguments unless isAsar

    if typeof cache is 'function'
      callback = cache
      cache = undefined

    archive = getOrCreateArchive asarPath
    return callback new Error("Invalid package #{asarPath}") unless archive

    real = archive.realpath filePath
    return callback createNotFoundError(asarPath, filePath) if real is false

    realpath asarPath, (err, p) ->
      return callback err if err
      callback null, path.join(p, real)

  exists = fs.exists
  fs.exists = (p, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return exists p, callback unless isAsar

    archive = getOrCreateArchive asarPath
    return callback new Error("Invalid package #{asarPath}") unless archive

    process.nextTick -> callback archive.stat(filePath) isnt false

  existsSync = fs.existsSync
  fs.existsSync = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return existsSync p unless isAsar

    archive = getOrCreateArchive asarPath
    return false unless archive

    archive.stat(filePath) isnt false

  open = fs.open
  readFile = fs.readFile
  fs.readFile = (p, options, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return readFile.apply this, arguments unless isAsar

    if typeof options is 'function'
      callback = options
      options = undefined

    archive = getOrCreateArchive asarPath
    return callback new Error("Invalid package #{asarPath}") unless archive

    info = archive.getFileInfo filePath
    return callback createNotFoundError(asarPath, filePath) unless info

    if not options
      options = encoding: null, flag: 'r'
    else if util.isString options
      options = encoding: options, flag: 'r'
    else if not util.isObject options
      throw new TypeError('Bad arguments')

    flag = options.flag || 'r'
    encoding = options.encoding

    buffer = new Buffer(info.size)
    open archive.path, flag, (error, fd) ->
      return callback error if error
      fs.read fd, buffer, 0, info.size, info.offset, (error) ->
        fs.close fd, ->
          callback error, if encoding then buffer.toString encoding else buffer

  openSync = fs.openSync
  readFileSync = fs.readFileSync
  fs.readFileSync = (p, options) ->
    [isAsar, asarPath, filePath] = splitPath p
    return readFileSync.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
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
    fd = openSync archive.path, flag
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

    archive = getOrCreateArchive asarPath
    return callback new Error("Invalid package #{asarPath}") unless archive

    files = archive.readdir filePath
    return callback createNotFoundError(asarPath, filePath) unless files

    process.nextTick -> callback null, files

  readdirSync = fs.readdirSync
  fs.readdirSync = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return readdirSync.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    throw new Error("Invalid package #{asarPath}") unless archive

    files = archive.readdir filePath
    throw createNotFoundError(asarPath, filePath) unless files

    files

  overrideAPI fs, 'open'
  overrideAPI child_process, 'execFile'
  overrideAPISync process, 'dlopen', 1
  overrideAPISync require('module')._extensions, '.node', 1
  overrideAPISync fs, 'openSync'
  overrideAPISync child_process, 'fork'
