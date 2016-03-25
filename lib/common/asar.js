(function () {
  const asar = process.binding('atom_common_asar')
  const child_process = require('child_process')
  const path = require('path')
  const util = require('util')

  var hasProp = {}.hasOwnProperty

  // Cache asar archive objects.
  var cachedArchives = {}

  var getOrCreateArchive = function (p) {
    var archive
    archive = cachedArchives[p]
    if (archive != null) {
      return archive
    }
    archive = asar.createArchive(p)
    if (!archive) {
      return false
    }
    cachedArchives[p] = archive
    return archive
  }

  // Clean cache on quit.
  process.on('exit', function () {
    var archive, p
    for (p in cachedArchives) {
      if (!hasProp.call(cachedArchives, p)) continue
      archive = cachedArchives[p]
      archive.destroy()
    }
  })

  // Separate asar package's path from full path.
  var splitPath = function (p) {
    var index

    // shortcut to disable asar.
    if (process.noAsar) {
      return [false]
    }

    if (typeof p !== 'string') {
      return [false]
    }
    if (p.substr(-5) === '.asar') {
      return [true, p, '']
    }
    p = path.normalize(p)
    index = p.lastIndexOf('.asar' + path.sep)
    if (index === -1) {
      return [false]
    }
    return [true, p.substr(0, index + 5), p.substr(index + 6)]
  }

  // Convert asar archive's Stats object to fs's Stats object.
  var nextInode = 0

  var uid = process.getuid != null ? process.getuid() : 0

  var gid = process.getgid != null ? process.getgid() : 0

  var fakeTime = new Date()

  var asarStatsToFsStats = function (stats) {
    return {
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
      isFile: function () {
        return stats.isFile
      },
      isDirectory: function () {
        return stats.isDirectory
      },
      isSymbolicLink: function () {
        return stats.isLink
      },
      isBlockDevice: function () {
        return false
      },
      isCharacterDevice: function () {
        return false
      },
      isFIFO: function () {
        return false
      },
      isSocket: function () {
        return false
      }
    }
  }

  // Create a ENOENT error.
  var notFoundError = function (asarPath, filePath, callback) {
    var error
    error = new Error(`ENOENT, ${filePath} not found in ${asarPath}`)
    error.code = 'ENOENT'
    error.errno = -2
    if (typeof callback !== 'function') {
      throw error
    }
    return process.nextTick(function () {
      return callback(error)
    })
  }

  // Create a ENOTDIR error.
  var notDirError = function (callback) {
    var error
    error = new Error('ENOTDIR, not a directory')
    error.code = 'ENOTDIR'
    error.errno = -20
    if (typeof callback !== 'function') {
      throw error
    }
    return process.nextTick(function () {
      return callback(error)
    })
  }

  // Create invalid archive error.
  var invalidArchiveError = function (asarPath, callback) {
    var error
    error = new Error(`Invalid package ${asarPath}`)
    if (typeof callback !== 'function') {
      throw error
    }
    return process.nextTick(function () {
      return callback(error)
    })
  }

  // Override APIs that rely on passing file path instead of content to C++.
  var overrideAPISync = function (module, name, arg) {
    var old
    if (arg == null) {
      arg = 0
    }
    old = module[name]
    module[name] = function () {
      var archive, newPath, p
      p = arguments[arg]
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return old.apply(this, arguments)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        invalidArchiveError(asarPath)
      }
      newPath = archive.copyFileOut(filePath)
      if (!newPath) {
        notFoundError(asarPath, filePath)
      }
      arguments[arg] = newPath
      return old.apply(this, arguments)
    }
  }

  var overrideAPI = function (module, name, arg) {
    var old
    if (arg == null) {
      arg = 0
    }
    old = module[name]
    module[name] = function () {
      var archive, callback, newPath, p
      p = arguments[arg]
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return old.apply(this, arguments)
      }
      callback = arguments[arguments.length - 1]
      if (typeof callback !== 'function') {
        return overrideAPISync(module, name, arg)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return invalidArchiveError(asarPath, callback)
      }
      newPath = archive.copyFileOut(filePath)
      if (!newPath) {
        return notFoundError(asarPath, filePath, callback)
      }
      arguments[arg] = newPath
      return old.apply(this, arguments)
    }
  }

  // Override fs APIs.
  exports.wrapFsWithAsar = function (fs) {
    var exists, existsSync, internalModuleReadFile, internalModuleStat, lstat, lstatSync, mkdir, mkdirSync, readFile, readFileSync, readdir, readdirSync, realpath, realpathSync, stat, statSync, statSyncNoException, logFDs, logASARAccess

    logFDs = {}
    logASARAccess = function (asarPath, filePath, offset) {
      if (!process.env.ELECTRON_LOG_ASAR_READS) {
        return
      }
      if (!logFDs[asarPath]) {
        var logFilename, logPath
        const path = require('path')
        logFilename = path.basename(asarPath, '.asar') + '-access-log.txt'
        logPath = path.join(require('os').tmpdir(), logFilename)
        logFDs[asarPath] = fs.openSync(logPath, 'a')
        console.log('Logging ' + asarPath + ' access to ' + logPath)
      }
      fs.writeSync(logFDs[asarPath], offset + ': ' + filePath + '\n')
    }

    lstatSync = fs.lstatSync
    fs.lstatSync = function (p) {
      var archive, stats
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return lstatSync(p)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        invalidArchiveError(asarPath)
      }
      stats = archive.stat(filePath)
      if (!stats) {
        notFoundError(asarPath, filePath)
      }
      return asarStatsToFsStats(stats)
    }
    lstat = fs.lstat
    fs.lstat = function (p, callback) {
      var archive, stats
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return lstat(p, callback)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return invalidArchiveError(asarPath, callback)
      }
      stats = getOrCreateArchive(asarPath).stat(filePath)
      if (!stats) {
        return notFoundError(asarPath, filePath, callback)
      }
      return process.nextTick(function () {
        return callback(null, asarStatsToFsStats(stats))
      })
    }
    statSync = fs.statSync
    fs.statSync = function (p) {
      const [isAsar] = splitPath(p)
      if (!isAsar) {
        return statSync(p)
      }

      // Do not distinguish links for now.
      return fs.lstatSync(p)
    }
    stat = fs.stat
    fs.stat = function (p, callback) {
      const [isAsar] = splitPath(p)
      if (!isAsar) {
        return stat(p, callback)
      }

      // Do not distinguish links for now.
      return process.nextTick(function () {
        return fs.lstat(p, callback)
      })
    }
    statSyncNoException = fs.statSyncNoException
    fs.statSyncNoException = function (p) {
      var archive, stats
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return statSyncNoException(p)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return false
      }
      stats = archive.stat(filePath)
      if (!stats) {
        return false
      }
      return asarStatsToFsStats(stats)
    }
    realpathSync = fs.realpathSync
    fs.realpathSync = function (p) {
      var archive, real
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return realpathSync.apply(this, arguments)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        invalidArchiveError(asarPath)
      }
      real = archive.realpath(filePath)
      if (real === false) {
        notFoundError(asarPath, filePath)
      }
      return path.join(realpathSync(asarPath), real)
    }
    realpath = fs.realpath
    fs.realpath = function (p, cache, callback) {
      var archive, real
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return realpath.apply(this, arguments)
      }
      if (typeof cache === 'function') {
        callback = cache
        cache = void 0
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return invalidArchiveError(asarPath, callback)
      }
      real = archive.realpath(filePath)
      if (real === false) {
        return notFoundError(asarPath, filePath, callback)
      }
      return realpath(asarPath, function (err, p) {
        if (err) {
          return callback(err)
        }
        return callback(null, path.join(p, real))
      })
    }
    exists = fs.exists
    fs.exists = function (p, callback) {
      var archive
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return exists(p, callback)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return invalidArchiveError(asarPath, callback)
      }
      return process.nextTick(function () {
        return callback(archive.stat(filePath) !== false)
      })
    }
    existsSync = fs.existsSync
    fs.existsSync = function (p) {
      var archive
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return existsSync(p)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return false
      }
      return archive.stat(filePath) !== false
    }
    readFile = fs.readFile
    fs.readFile = function (p, options, callback) {
      var archive, buffer, encoding, fd, info, realPath
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return readFile.apply(this, arguments)
      }
      if (typeof options === 'function') {
        callback = options
        options = void 0
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return invalidArchiveError(asarPath, callback)
      }
      info = archive.getFileInfo(filePath)
      if (!info) {
        return notFoundError(asarPath, filePath, callback)
      }
      if (info.size === 0) {
        return process.nextTick(function () {
          return callback(null, new Buffer(0))
        })
      }
      if (info.unpacked) {
        realPath = archive.copyFileOut(filePath)
        return fs.readFile(realPath, options, callback)
      }
      if (!options) {
        options = {
          encoding: null
        }
      } else if (util.isString(options)) {
        options = {
          encoding: options
        }
      } else if (!util.isObject(options)) {
        throw new TypeError('Bad arguments')
      }
      encoding = options.encoding
      buffer = new Buffer(info.size)
      fd = archive.getFd()
      if (!(fd >= 0)) {
        return notFoundError(asarPath, filePath, callback)
      }
      logASARAccess(asarPath, filePath, info.offset)
      return fs.read(fd, buffer, 0, info.size, info.offset, function (error) {
        return callback(error, encoding ? buffer.toString(encoding) : buffer)
      })
    }
    readFileSync = fs.readFileSync
    fs.readFileSync = function (p, opts) {
      // this allows v8 to optimize this function
      var archive, buffer, encoding, fd, info, options, realPath
      options = opts
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return readFileSync.apply(this, arguments)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        invalidArchiveError(asarPath)
      }
      info = archive.getFileInfo(filePath)
      if (!info) {
        notFoundError(asarPath, filePath)
      }
      if (info.size === 0) {
        if (options) {
          return ''
        } else {
          return new Buffer(0)
        }
      }
      if (info.unpacked) {
        realPath = archive.copyFileOut(filePath)
        return fs.readFileSync(realPath, options)
      }
      if (!options) {
        options = {
          encoding: null
        }
      } else if (util.isString(options)) {
        options = {
          encoding: options
        }
      } else if (!util.isObject(options)) {
        throw new TypeError('Bad arguments')
      }
      encoding = options.encoding
      buffer = new Buffer(info.size)
      fd = archive.getFd()
      if (!(fd >= 0)) {
        notFoundError(asarPath, filePath)
      }
      logASARAccess(asarPath, filePath, info.offset)
      fs.readSync(fd, buffer, 0, info.size, info.offset)
      if (encoding) {
        return buffer.toString(encoding)
      } else {
        return buffer
      }
    }
    readdir = fs.readdir
    fs.readdir = function (p, callback) {
      var archive, files
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return readdir.apply(this, arguments)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return invalidArchiveError(asarPath, callback)
      }
      files = archive.readdir(filePath)
      if (!files) {
        return notFoundError(asarPath, filePath, callback)
      }
      return process.nextTick(function () {
        return callback(null, files)
      })
    }
    readdirSync = fs.readdirSync
    fs.readdirSync = function (p) {
      var archive, files
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return readdirSync.apply(this, arguments)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        invalidArchiveError(asarPath)
      }
      files = archive.readdir(filePath)
      if (!files) {
        notFoundError(asarPath, filePath)
      }
      return files
    }
    internalModuleReadFile = process.binding('fs').internalModuleReadFile
    process.binding('fs').internalModuleReadFile = function (p) {
      var archive, buffer, fd, info, realPath
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return internalModuleReadFile(p)
      }
      archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return void 0
      }
      info = archive.getFileInfo(filePath)
      if (!info) {
        return void 0
      }
      if (info.size === 0) {
        return ''
      }
      if (info.unpacked) {
        realPath = archive.copyFileOut(filePath)
        return fs.readFileSync(realPath, {
          encoding: 'utf8'
        })
      }
      buffer = new Buffer(info.size)
      fd = archive.getFd()
      if (!(fd >= 0)) {
        return void 0
      }
      logASARAccess(asarPath, filePath, info.offset)
      fs.readSync(fd, buffer, 0, info.size, info.offset)
      return buffer.toString('utf8')
    }
    internalModuleStat = process.binding('fs').internalModuleStat
    process.binding('fs').internalModuleStat = function (p) {
      var archive, stats
      const [isAsar, asarPath, filePath] = splitPath(p)
      if (!isAsar) {
        return internalModuleStat(p)
      }
      archive = getOrCreateArchive(asarPath)

      // -ENOENT
      if (!archive) {
        return -34
      }
      stats = archive.stat(filePath)

      // -ENOENT
      if (!stats) {
        return -34
      }
      if (stats.isDirectory) {
        return 1
      } else {
        return 0
      }
    }

    // Calling mkdir for directory inside asar archive should throw ENOTDIR
    // error, but on Windows it throws ENOENT.
    // This is to work around the recursive looping bug of mkdirp since it is
    // widely used.
    if (process.platform === 'win32') {
      mkdir = fs.mkdir
      fs.mkdir = function (p, mode, callback) {
        if (typeof mode === 'function') {
          callback = mode
        }
        const [isAsar, , filePath] = splitPath(p)
        if (isAsar && filePath.length) {
          return notDirError(callback)
        }
        return mkdir(p, mode, callback)
      }
      mkdirSync = fs.mkdirSync
      fs.mkdirSync = function (p, mode) {
        const [isAsar, , filePath] = splitPath(p)
        if (isAsar && filePath.length) {
          notDirError()
        }
        return mkdirSync(p, mode)
      }
    }
    overrideAPI(fs, 'open')
    overrideAPI(child_process, 'execFile')
    overrideAPISync(process, 'dlopen', 1)
    overrideAPISync(require('module')._extensions, '.node', 1)
    overrideAPISync(fs, 'openSync')
    return overrideAPISync(child_process, 'execFileSync')
  }
})()
