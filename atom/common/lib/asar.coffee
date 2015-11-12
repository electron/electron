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
  archive.destroy() for own p, archive of cachedArchives

# Separate asar package's path from full path.
splitPath = (p) ->
  return [false] if typeof p isnt 'string'
  return [true, p, ''] if p.substr(-5) is '.asar'
  p = path.normalize p
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
notFoundError = (asarPath, filePath, callback) ->
  error = new Error("ENOENT, #{filePath} not found in #{asarPath}")
  error.code = "ENOENT"
  error.errno = -2
  unless typeof callback is 'function'
    throw error
  process.nextTick -> callback error

# Create invalid archive error.
invalidArchiveError = (asarPath, callback) ->
  error = new Error("Invalid package #{asarPath}")
  unless typeof callback is 'function'
    throw error
  process.nextTick -> callback error

# Override APIs that rely on passing file path instead of content to C++.
overrideAPISync = (module, name, arg = 0) ->
  old = module[name]
  module[name] = ->
    p = arguments[arg]
    [isAsar, asarPath, filePath] = splitPath p
    return old.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    invalidArchiveError asarPath unless archive

    newPath = archive.copyFileOut filePath
    notFoundError asarPath, filePath unless newPath

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
    return invalidArchiveError asarPath, callback unless archive

    newPath = archive.copyFileOut filePath
    return notFoundError asarPath, filePath, callback unless newPath

    arguments[arg] = newPath
    old.apply this, arguments

# Override fs APIs.
exports.wrapFsWithAsar = (fs) ->
  lstatSync = fs.lstatSync
  fs.lstatSync = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return lstatSync p unless isAsar

    archive = getOrCreateArchive asarPath
    invalidArchiveError asarPath unless archive

    stats = archive.stat filePath
    notFoundError asarPath, filePath unless stats

    asarStatsToFsStats stats

  lstat = fs.lstat
  fs.lstat = (p, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return lstat p, callback unless isAsar

    archive = getOrCreateArchive asarPath
    return invalidArchiveError asarPath, callback unless archive

    stats = getOrCreateArchive(asarPath).stat filePath
    return notFoundError asarPath, filePath, callback unless stats

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
    invalidArchiveError asarPath unless archive

    real = archive.realpath filePath
    notFoundError asarPath, filePath if real is false

    path.join realpathSync(asarPath), real

  realpath = fs.realpath
  fs.realpath = (p, cache, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return realpath.apply this, arguments unless isAsar

    if typeof cache is 'function'
      callback = cache
      cache = undefined

    archive = getOrCreateArchive asarPath
    return invalidArchiveError asarPath, callback unless archive

    real = archive.realpath filePath
    if real is false
      return notFoundError asarPath, filePath, callback

    realpath asarPath, (err, p) ->
      return callback err if err
      callback null, path.join(p, real)

  exists = fs.exists
  fs.exists = (p, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return exists p, callback unless isAsar

    archive = getOrCreateArchive asarPath
    return invalidArchiveError asarPath, callback unless archive

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
    return invalidArchiveError asarPath, callback unless archive

    info = archive.getFileInfo filePath
    return notFoundError asarPath, filePath, callback unless info

    if info.size is 0
      return process.nextTick -> callback null, new Buffer(0)

    if info.unpacked
      realPath = archive.copyFileOut filePath
      return fs.readFile realPath, options, callback

    if not options
      options = encoding: null
    else if util.isString options
      options = encoding: options
    else if not util.isObject options
      throw new TypeError('Bad arguments')

    encoding = options.encoding

    buffer = new Buffer(info.size)
    fd = archive.getFd()
    return notFoundError asarPath, filePath, callback unless fd >= 0

    fs.read fd, buffer, 0, info.size, info.offset, (error) ->
      callback error, if encoding then buffer.toString encoding else buffer

  openSync = fs.openSync
  readFileSync = fs.readFileSync
  fs.readFileSync = (p, opts) ->
    options = opts # this allows v8 to optimize this function
    [isAsar, asarPath, filePath] = splitPath p
    return readFileSync.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    invalidArchiveError asarPath unless archive

    info = archive.getFileInfo filePath
    notFoundError asarPath, filePath unless info

    if info.size is 0
      return if options then '' else new Buffer(0)

    if info.unpacked
      realPath = archive.copyFileOut filePath
      return fs.readFileSync realPath, options

    if not options
      options = encoding: null
    else if util.isString options
      options = encoding: options
    else if not util.isObject options
      throw new TypeError('Bad arguments')

    encoding = options.encoding

    buffer = new Buffer(info.size)
    fd = archive.getFd()
    notFoundError asarPath, filePath unless fd >= 0

    fs.readSync fd, buffer, 0, info.size, info.offset
    if encoding then buffer.toString encoding else buffer

  readdir = fs.readdir
  fs.readdir = (p, callback) ->
    [isAsar, asarPath, filePath] = splitPath p
    return readdir.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    return invalidArchiveError asarPath, callback unless archive

    files = archive.readdir filePath
    return notFoundError asarPath, filePath, callback unless files

    process.nextTick -> callback null, files

  readdirSync = fs.readdirSync
  fs.readdirSync = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return readdirSync.apply this, arguments unless isAsar

    archive = getOrCreateArchive asarPath
    invalidArchiveError asarPath unless archive

    files = archive.readdir filePath
    notFoundError asarPath, filePath unless files

    files

  internalModuleReadFile = process.binding('fs').internalModuleReadFile
  process.binding('fs').internalModuleReadFile = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return internalModuleReadFile p unless isAsar

    archive = getOrCreateArchive asarPath
    return undefined unless archive

    info = archive.getFileInfo filePath
    return undefined unless info
    return '' if info.size is 0

    if info.unpacked
      realPath = archive.copyFileOut filePath
      return fs.readFileSync realPath, encoding: 'utf8'

    buffer = new Buffer(info.size)
    fd = archive.getFd()
    return undefined unless fd >= 0

    fs.readSync fd, buffer, 0, info.size, info.offset
    buffer.toString 'utf8'

  internalModuleStat = process.binding('fs').internalModuleStat
  process.binding('fs').internalModuleStat = (p) ->
    [isAsar, asarPath, filePath] = splitPath p
    return internalModuleStat p unless isAsar

    archive = getOrCreateArchive asarPath
    return -34 unless archive  # -ENOENT

    stats = archive.stat filePath
    return -34 unless stats  # -ENOENT

    if stats.isDirectory then return 1 else return 0

  overrideAPI fs, 'open'
  overrideAPI child_process, 'execFile'
  overrideAPISync process, 'dlopen', 1
  overrideAPISync require('module')._extensions, '.node', 1
  overrideAPISync fs, 'openSync'
