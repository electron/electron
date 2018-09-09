(function () {
  const asar = process.binding('atom_common_asar')
  const assert = require('assert')
  const {Buffer} = require('buffer')
  const childProcess = require('child_process')
  const path = require('path')
  const util = require('util')

  // asar error types
  const NOT_FOUND = 'NOT_FOUND'
  const NOT_DIR = 'NOT_DIR'
  const NO_ACCESS = 'NO_ACCESS'
  const INVALID_ARCHIVE = 'INVALID_ARCHIVE'

  const envNoAsar = process.env.ELECTRON_NO_ASAR &&
      process.type !== 'browser' &&
      process.type !== 'renderer'
  const isAsarDisabled = () => process.noAsar || envNoAsar

  // Cache asar archive objects.
  const cachedArchives = new Map()

  const getOrCreateArchive = archivePath => {
    const isCached = cachedArchives.has(archivePath)
    if (isCached) {
      return cachedArchives.get(archivePath)
    }

    const newArchive = asar.createArchive(archivePath)
    if (!newArchive) return null

    cachedArchives.set(archivePath, newArchive)
    return newArchive
  }

  // Clean cache on quit.
  process.on('exit', () => {
    for (const archive of cachedArchives.values()) {
      archive.destroy()
    }
    cachedArchives.clear()
  })

  const ASAR_EXTENSION = '.asar'

  // Separate asar package's path from full path.
  const splitPath = archivePathOrBuffer => {
    // Shortcut for disabled asar.
    if (isAsarDisabled()) return {isAsar: false}

    // Check for a bad argument type.
    let archivePath = archivePathOrBuffer
    if (Buffer.isBuffer(archivePathOrBuffer)) {
      archivePath = archivePathOrBuffer.toString()
    }
    if (typeof archivePath !== 'string') return {isAsar: false}

    if (archivePath.endsWith(ASAR_EXTENSION)) {
      return {isAsar: true, asarPath: archivePath, filePath: ''}
    }

    archivePath = path.normalize(archivePath)
    const index = archivePath.lastIndexOf(`${ASAR_EXTENSION}${path.sep}`)
    if (index === -1) return {isAsar: false}

    // E.g. for "//some/path/to/archive.asar/then/internal.file"...
    return {
      isAsar: true,
      // "//some/path/to/archive.asar"
      asarPath: archivePath.substr(0, index + ASAR_EXTENSION.length),
      // "then/internal.file" (with a path separator excluded)
      filePath: archivePath.substr(index + ASAR_EXTENSION.length + 1)
    }
  }

  // Convert asar archive's Stats object to fs's Stats object.
  let nextInode = 0

  const uid = process.getuid != null ? process.getuid() : 0
  const gid = process.getgid != null ? process.getgid() : 0

  const fakeTime = new Date()
  const msec = (date) => (date || fakeTime).getTime()

  const asarStatsToFsStats = function (stats) {
    const {Stats, constants} = require('fs')

    let mode = constants.S_IROTH ^ constants.S_IRGRP ^ constants.S_IRUSR ^ constants.S_IWUSR

    if (stats.isFile) {
      mode ^= constants.S_IFREG
    } else if (stats.isDirectory) {
      mode ^= constants.S_IFDIR
    } else if (stats.isLink) {
      mode ^= constants.S_IFLNK
    }

    return new Stats(
        1, // dev
        mode, // mode
        1, // nlink
        uid,
        gid,
        0, // rdev
        undefined, // blksize
        ++nextInode, // ino
        stats.size,
        undefined, // blocks,
        msec(stats.atime), // atim_msec
        msec(stats.mtime), // mtim_msec
        msec(stats.ctime), // ctim_msec
        msec(stats.birthtime) // birthtim_msec
    )
  }

  const createError = (errorType, {asarPath, filePath, callback} = {}) => {
    let error
    switch (errorType) {
      case NOT_FOUND:
        error = new Error(`ENOENT, ${filePath} not found in ${asarPath}`)
        error.code = 'ENOENT'
        error.errno = -2
        break
      case NOT_DIR:
        error = new Error('ENOTDIR, not a directory')
        error.code = 'ENOTDIR'
        error.errno = -20
        break
      case NO_ACCESS:
        error = new Error(`EACCES: permission denied, access '${filePath}'`)
        error.code = 'EACCES'
        error.errno = -13
        break
      case INVALID_ARCHIVE:
        error = new Error(`Invalid package ${asarPath}`)
        break
      default:
        assert.fail(`Invalid error type "${errorType}" passed to createError.`)
    }
    if (typeof callback !== 'function') throw error
    process.nextTick(() => callback(error))
  }

  const overrideAPISync = function (module, name, arg) {
    if (arg == null) arg = 0
    const old = module[name]
    module[name] = function () {
      const p = arguments[arg]
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return old.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) createError(INVALID_ARCHIVE, {asarPath})

      const newPath = archive.copyFileOut(filePath)
      if (!newPath) createError(NOT_FOUND, {asarPath, filePath})

      arguments[arg] = newPath
      return old.apply(this, arguments)
    }
  }

  const overrideAPI = function (module, name, arg) {
    if (arg == null) arg = 0
    const old = module[name]
    module[name] = function () {
      const p = arguments[arg]
      const {isAsar, asarPath, filePath} = splitPath(p)
      console.log(`${isAsar}, ${asarPath}, ${filePath}`)
      if (!isAsar) return old.apply(this, arguments)

      const callback = arguments[arguments.length - 1]
      if (typeof callback !== 'function') return overrideAPISync(module, name, arg)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const newPath = archive.copyFileOut(filePath)
      if (!newPath) return createError(NOT_FOUND, {asarPath, filePath, callback})

      arguments[arg] = newPath
      return old.apply(this, arguments)
    }

    if (old[util.promisify.custom]) {
      module[name][util.promisify.custom] = function () {
        const p = arguments[arg]
        const {isAsar, asarPath, filePath} = splitPath(p)
        if (!isAsar) return old[util.promisify.custom].apply(this, arguments)

        const archive = getOrCreateArchive(asarPath)
        if (!archive) return new Promise(() => createError(INVALID_ARCHIVE, {asarPath}))

        const newPath = archive.copyFileOut(filePath)
        if (!newPath) return new Promise(() => createError(NOT_FOUND, {asarPath, filePath}))

        arguments[arg] = newPath
        return old[util.promisify.custom].apply(this, arguments)
      }
    }
  }

  // Override fs APIs.
  exports.wrapFsWithAsar = fs => {
    const logFDs = {}
    const logASARAccess = (asarPath, filePath, offset) => {
      if (!process.env.ELECTRON_LOG_ASAR_READS) return
      if (!logFDs[asarPath]) {
        const path = require('path')
        const logFilename = `${path.basename(asarPath, '.asar')}-access-log.txt`
        const logPath = path.join(require('os').tmpdir(), logFilename)
        logFDs[asarPath] = fs.openSync(logPath, 'a')
        console.log(`Logging ${asarPath} access to ${logPath}`)
      }
      fs.writeSync(logFDs[asarPath], `${offset}: ${filePath}\n`)
    }

    const {lstatSync} = fs
    fs.lstatSync = p => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return lstatSync(p)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) createError(INVALID_ARCHIVE, {asarPath})

      const stats = archive.stat(filePath)
      if (!stats) createError(NOT_FOUND, {asarPath, filePath})
      return asarStatsToFsStats(stats)
    }

    const {lstat} = fs
    fs.lstat = (p, callback) => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return lstat(p, callback)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const stats = getOrCreateArchive(asarPath).stat(filePath)
      if (!stats) return createError(NOT_FOUND, {asarPath, filePath, callback})

      process.nextTick(() => callback(null, asarStatsToFsStats(stats)))
    }

    const {statSync} = fs
    fs.statSync = p => {
      const {isAsar} = splitPath(p)
      if (!isAsar) return statSync(p)

      // Do not distinguish links for now.
      return fs.lstatSync(p)
    }

    const {stat} = fs
    fs.stat = (p, callback) => {
      const {isAsar} = splitPath(p)
      if (!isAsar) return stat(p, callback)

      // Do not distinguish links for now.
      process.nextTick(() => fs.lstat(p, callback))
    }

    const {statSyncNoException} = fs
    fs.statSyncNoException = p => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return statSyncNoException(p)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return false

      const stats = archive.stat(filePath)
      if (!stats) return false

      return asarStatsToFsStats(stats)
    }

    const {realpathSync} = fs
    fs.realpathSync = function (p) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return realpathSync.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath})

      const real = archive.realpath(filePath)
      if (real === false) createError(NOT_FOUND, {asarPath, filePath})

      return path.join(realpathSync(asarPath), real)
    }

    fs.realpathSync.native = function (p) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return realpathSync.native.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath})

      const real = archive.realpath(filePath)
      if (real === false) createError(NOT_FOUND, {asarPath, filePath})

      return path.join(realpathSync.native(asarPath), real)
    }

    const {realpath} = fs
    fs.realpath = function (p, cache, callback) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return realpath.apply(this, arguments)

      if (typeof cache === 'function') {
        callback = cache
        cache = undefined
      }

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const real = archive.realpath(filePath)
      if (real === false) return createError(NOT_FOUND, {asarPath, filePath, callback})

      return realpath(asarPath, (err, p) => {
        return (err) ? callback(err) : callback(null, path.join(p, real))
      })
    }

    fs.realpath.native = function (p, cache, callback) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return realpath.native.apply(this, arguments)

      if (typeof cache === 'function') {
        callback = cache
        cache = void 0
      }

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const real = archive.realpath(filePath)
      if (real === false) return createError(NOT_FOUND, {asarPath, filePath, callback})

      return realpath.native(asarPath, (err, p) => {
        return (err) ? callback(err) : callback(null, path.join(p, real))
      })
    }

    const {exists} = fs
    fs.exists = (p, callback) => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return exists(p, callback)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      // Disabled due to false positive in StandardJS
      // eslint-disable-next-line standard/no-callback-literal
      process.nextTick(() => callback(archive.stat(filePath) !== false))
    }

    fs.exists[util.promisify.custom] = p => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return exists[util.promisify.custom](p)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return new Promise(() => createError(INVALID_ARCHIVE, {asarPath}))

      return Promise.resolve(archive.stat(filePath) !== false)
    }

    const {existsSync} = fs
    fs.existsSync = p => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return existsSync(p)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return false

      return archive.stat(filePath) !== false
    }

    const {access} = fs
    fs.access = function (p, mode, callback) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return access.apply(this, arguments)

      if (typeof mode === 'function') {
        callback = mode
        mode = fs.constants.F_OK
      }

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const info = archive.getFileInfo(filePath)
      if (!info) return createError(NOT_FOUND, {asarPath, filePath, callback})

      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.access(realPath, mode, callback)
      }

      const stats = getOrCreateArchive(asarPath).stat(filePath)
      if (!stats) return createError(NOT_FOUND, {asarPath, filePath, callback})

      if (mode & fs.constants.W_OK) return createError(NO_ACCESS, {asarPath, filePath, callback})

      process.nextTick(() => callback())
    }

    const {accessSync} = fs
    fs.accessSync = function (p, mode) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return accessSync.apply(this, arguments)
      if (mode == null) mode = fs.constants.F_OK

      const archive = getOrCreateArchive(asarPath)
      if (!archive) createError(INVALID_ARCHIVE, {asarPath})

      const info = archive.getFileInfo(filePath)
      if (!info) createError(NOT_FOUND, {asarPath, filePath})
      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.accessSync(realPath, mode)
      }

      const stats = getOrCreateArchive(asarPath).stat(filePath)
      if (!stats) createError(NOT_FOUND, {asarPath, filePath})
      if (mode & fs.constants.W_OK) createError(NO_ACCESS, {asarPath, filePath})
    }

    const {readFile} = fs
    fs.readFile = function (p, options, callback) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return readFile.apply(this, arguments)

      if (typeof options === 'function') {
        callback = options
        options = { encoding: null }
      } else if (typeof options === 'string') {
        options = { encoding: options }
      } else if (options === null || options === undefined) {
        options = { encoding: null }
      } else if (typeof options !== 'object') {
        throw new TypeError('Bad arguments')
      }

      const {encoding} = options
      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const info = archive.getFileInfo(filePath)
      if (!info) return createError(NOT_FOUND, {asarPath, filePath, callback})
      if (info.size === 0) return process.nextTick(() => callback(null, encoding ? '' : Buffer.alloc(0)))
      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.readFile(realPath, options, callback)
      }

      const buffer = Buffer.alloc(info.size)
      const fd = archive.getFd()
      if (!(fd >= 0)) return createError(NOT_FOUND, {asarPath, filePath, callback})

      logASARAccess(asarPath, filePath, info.offset)
      fs.read(fd, buffer, 0, info.size, info.offset, error => {
        callback(error, encoding ? buffer.toString(encoding) : buffer)
      })
    }

    const {readFileSync} = fs
    fs.readFileSync = function (p, options) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return readFileSync.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) createError(INVALID_ARCHIVE, {asarPath})

      const info = archive.getFileInfo(filePath)
      if (!info) createError(NOT_FOUND, {asarPath, filePath})
      if (info.size === 0) return (options) ? '' : Buffer.alloc(0)
      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.readFileSync(realPath, options)
      }

      if (!options) {
        options = { encoding: null }
      } else if (typeof options === 'string') {
        options = { encoding: options }
      } else if (typeof options !== 'object') {
        throw new TypeError('Bad arguments')
      }

      const {encoding} = options
      const buffer = Buffer.alloc(info.size)
      const fd = archive.getFd()
      if (!(fd >= 0)) createError(NOT_FOUND, {asarPath, filePath})

      logASARAccess(asarPath, filePath, info.offset)
      fs.readSync(fd, buffer, 0, info.size, info.offset)
      return (encoding) ? buffer.toString(encoding) : buffer
    }

    const {readdir} = fs
    fs.readdir = function (p, callback) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return readdir.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return createError(INVALID_ARCHIVE, {asarPath, callback})

      const files = archive.readdir(filePath)
      if (!files) return createError(NOT_FOUND, {asarPath, filePath, callback})

      process.nextTick(() => callback(null, files))
    }

    const {readdirSync} = fs
    fs.readdirSync = function (p) {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return readdirSync.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) createError(INVALID_ARCHIVE, {asarPath})

      const files = archive.readdir(filePath)
      if (!files) createError(NOT_FOUND, {asarPath, filePath})

      return files
    }

    const {internalModuleReadJSON} = process.binding('fs')
    process.binding('fs').internalModuleReadJSON = p => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return internalModuleReadJSON(p)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return

      const info = archive.getFileInfo(filePath)
      if (!info) return
      if (info.size === 0) return ''
      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.readFileSync(realPath, {encoding: 'utf8'})
      }

      const buffer = Buffer.alloc(info.size)
      const fd = archive.getFd()
      if (!(fd >= 0)) return

      logASARAccess(asarPath, filePath, info.offset)
      fs.readSync(fd, buffer, 0, info.size, info.offset)
      return buffer.toString('utf8')
    }

    const {internalModuleStat} = process.binding('fs')
    process.binding('fs').internalModuleStat = p => {
      const {isAsar, asarPath, filePath} = splitPath(p)
      if (!isAsar) return internalModuleStat(p)

      // -ENOENT
      const archive = getOrCreateArchive(asarPath)
      if (!archive) return -34

      // -ENOENT
      const stats = archive.stat(filePath)
      if (!stats) return -34

      return (stats.isDirectory) ? 1 : 0
    }

    // Calling mkdir for directory inside asar archive should throw ENOTDIR
    // error, but on Windows it throws ENOENT.
    // This is to work around the recursive looping bug of mkdirp since it is
    // widely used.
    if (process.platform === 'win32') {
      const {mkdir} = fs
      fs.mkdir = (p, mode, callback) => {
        if (typeof mode === 'function') callback = mode

        const {isAsar, filePath} = splitPath(p)
        if (isAsar && filePath.length) return createError(NOT_DIR, {callback})

        mkdir(p, mode, callback)
      }

      const {mkdirSync} = fs
      fs.mkdirSync = function (p, mode) {
        const {isAsar, filePath} = splitPath(p)
        if (isAsar && filePath.length) createError(NOT_DIR)
        return mkdirSync(p, mode)
      }
    }

    // Executing a command string containing a path to an asar
    // archive confuses `childProcess.execFile`, which is internally
    // called by `childProcess.{exec,execSync}`, causing
    // Electron to consider the full command as a single path
    // to an archive.
    const {exec, execSync} = childProcess
    childProcess.exec = invokeWithNoAsar(exec)
    childProcess.exec[util.promisify.custom] = invokeWithNoAsar(exec[util.promisify.custom])
    childProcess.execSync = invokeWithNoAsar(execSync)

    function invokeWithNoAsar (func) {
      return function () {
        const processNoAsarOriginalValue = process.noAsar
        process.noAsar = true
        try {
          return func.apply(this, arguments)
        } finally {
          process.noAsar = processNoAsarOriginalValue
        }
      }
    }

    overrideAPI(fs, 'open')
    overrideAPI(childProcess, 'execFile')
    overrideAPISync(process, 'dlopen', 1)
    overrideAPISync(require('module')._extensions, '.node', 1)
    overrideAPISync(fs, 'openSync')
    overrideAPISync(childProcess, 'execFileSync')
  }
})()
