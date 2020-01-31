'use strict';

(function () {
  const asar = process._linkedBinding('atom_common_asar')
  const v8Util = process._linkedBinding('atom_common_v8_util')
  const { Buffer } = require('buffer')
  const Module = require('module')
  const path = require('path')
  const util = require('util')

  const Promise = global.Promise

  const envNoAsar = process.env.ELECTRON_NO_ASAR &&
      process.type !== 'browser' &&
      process.type !== 'renderer'
  const isAsarDisabled = () => process.noAsar || envNoAsar

  const internalBinding = process.internalBinding
  delete process.internalBinding

  /**
   * @param {!Function} functionToCall
   * @param {!Array|undefined} args
   */
  const nextTick = (functionToCall, args = []) => {
    process.nextTick(() => functionToCall(...args))
  }

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

  // Separate asar package's path from full path.
  const splitPath = archivePathOrBuffer => {
    // Shortcut for disabled asar.
    if (isAsarDisabled()) return { isAsar: false }

    // Check for a bad argument type.
    let archivePath = archivePathOrBuffer
    if (Buffer.isBuffer(archivePathOrBuffer)) {
      archivePath = archivePathOrBuffer.toString()
    }
    if (typeof archivePath !== 'string') return { isAsar: false }

    return asar.splitPath(path.normalize(archivePath))
  }

  // Convert asar archive's Stats object to fs's Stats object.
  let nextInode = 0

  const uid = process.getuid != null ? process.getuid() : 0
  const gid = process.getgid != null ? process.getgid() : 0

  const fakeTime = new Date()
  const msec = (date) => (date || fakeTime).getTime()

  const asarStatsToFsStats = function (stats) {
    const { Stats, constants } = require('fs')

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

  const AsarError = {
    NOT_FOUND: 'NOT_FOUND',
    NOT_DIR: 'NOT_DIR',
    NO_ACCESS: 'NO_ACCESS',
    INVALID_ARCHIVE: 'INVALID_ARCHIVE'
  }

  const createError = (errorType, { asarPath, filePath } = {}) => {
    let error
    switch (errorType) {
      case AsarError.NOT_FOUND:
        error = new Error(`ENOENT, ${filePath} not found in ${asarPath}`)
        error.code = 'ENOENT'
        error.errno = -2
        break
      case AsarError.NOT_DIR:
        error = new Error('ENOTDIR, not a directory')
        error.code = 'ENOTDIR'
        error.errno = -20
        break
      case AsarError.NO_ACCESS:
        error = new Error(`EACCES: permission denied, access '${filePath}'`)
        error.code = 'EACCES'
        error.errno = -13
        break
      case AsarError.INVALID_ARCHIVE:
        error = new Error(`Invalid package ${asarPath}`)
        break
      default:
        throw new Error(`Invalid error type "${errorType}" passed to createError.`)
    }
    return error
  }

  const overrideAPISync = function (module, name, pathArgumentIndex, fromAsync) {
    if (pathArgumentIndex == null) pathArgumentIndex = 0
    const old = module[name]
    const func = function () {
      const pathArgument = arguments[pathArgumentIndex]
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return old.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) throw createError(AsarError.INVALID_ARCHIVE, { asarPath })

      const newPath = archive.copyFileOut(filePath)
      if (!newPath) throw createError(AsarError.NOT_FOUND, { asarPath, filePath })

      arguments[pathArgumentIndex] = newPath
      return old.apply(this, arguments)
    }
    if (fromAsync) {
      return func
    }
    module[name] = func
  }

  const overrideAPI = function (module, name, pathArgumentIndex) {
    if (pathArgumentIndex == null) pathArgumentIndex = 0
    const old = module[name]
    module[name] = function () {
      const pathArgument = arguments[pathArgumentIndex]
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return old.apply(this, arguments)

      const callback = arguments[arguments.length - 1]
      if (typeof callback !== 'function') {
        return overrideAPISync(module, name, pathArgumentIndex, true).apply(this, arguments)
      }

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        nextTick(callback, [error])
        return
      }

      const newPath = archive.copyFileOut(filePath)
      if (!newPath) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      arguments[pathArgumentIndex] = newPath
      return old.apply(this, arguments)
    }

    if (old[util.promisify.custom]) {
      module[name][util.promisify.custom] = makePromiseFunction(old[util.promisify.custom], pathArgumentIndex)
    }

    if (module.promises && module.promises[name]) {
      module.promises[name] = makePromiseFunction(module.promises[name], pathArgumentIndex)
    }
  }

  const makePromiseFunction = function (orig, pathArgumentIndex) {
    return function (...args) {
      const pathArgument = args[pathArgumentIndex]
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return orig.apply(this, args)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        return Promise.reject(createError(AsarError.INVALID_ARCHIVE, { asarPath }))
      }

      const newPath = archive.copyFileOut(filePath)
      if (!newPath) {
        return Promise.reject(createError(AsarError.NOT_FOUND, { asarPath, filePath }))
      }

      args[pathArgumentIndex] = newPath
      return orig.apply(this, args)
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
      }
      fs.writeSync(logFDs[asarPath], `${offset}: ${filePath}\n`)
    }

    const { lstatSync } = fs
    fs.lstatSync = (pathArgument, options) => {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return lstatSync(pathArgument, options)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) throw createError(AsarError.INVALID_ARCHIVE, { asarPath })

      const stats = archive.stat(filePath)
      if (!stats) throw createError(AsarError.NOT_FOUND, { asarPath, filePath })

      return asarStatsToFsStats(stats)
    }

    const { lstat } = fs
    fs.lstat = function (pathArgument, options, callback) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (typeof options === 'function') {
        callback = options
        options = {}
      }
      if (!isAsar) return lstat(pathArgument, options, callback)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        nextTick(callback, [error])
        return
      }

      const stats = archive.stat(filePath)
      if (!stats) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      const fsStats = asarStatsToFsStats(stats)
      nextTick(callback, [null, fsStats])
    }

    fs.promises.lstat = util.promisify(fs.lstat)

    const { statSync } = fs
    fs.statSync = (pathArgument, options) => {
      const { isAsar } = splitPath(pathArgument)
      if (!isAsar) return statSync(pathArgument, options)

      // Do not distinguish links for now.
      return fs.lstatSync(pathArgument, options)
    }

    const { stat } = fs
    fs.stat = (pathArgument, options, callback) => {
      const { isAsar } = splitPath(pathArgument)
      if (typeof options === 'function') {
        callback = options
        options = {}
      }
      if (!isAsar) return stat(pathArgument, options, callback)

      // Do not distinguish links for now.
      process.nextTick(() => fs.lstat(pathArgument, options, callback))
    }

    fs.promises.stat = util.promisify(fs.stat)

    const wrapRealpathSync = function (realpathSync) {
      return function (pathArgument, options) {
        const { isAsar, asarPath, filePath } = splitPath(pathArgument)
        if (!isAsar) return realpathSync.apply(this, arguments)

        const archive = getOrCreateArchive(asarPath)
        if (!archive) {
          throw createError(AsarError.INVALID_ARCHIVE, { asarPath })
        }

        const fileRealPath = archive.realpath(filePath)
        if (fileRealPath === false) {
          throw createError(AsarError.NOT_FOUND, { asarPath, filePath })
        }

        return path.join(realpathSync(asarPath, options), fileRealPath)
      }
    }

    const { realpathSync } = fs
    fs.realpathSync = wrapRealpathSync(realpathSync)
    fs.realpathSync.native = wrapRealpathSync(realpathSync.native)

    const wrapRealpath = function (realpath) {
      return function (pathArgument, options, callback) {
        const { isAsar, asarPath, filePath } = splitPath(pathArgument)
        if (!isAsar) return realpath.apply(this, arguments)

        if (arguments.length < 3) {
          callback = options
          options = {}
        }

        const archive = getOrCreateArchive(asarPath)
        if (!archive) {
          const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
          nextTick(callback, [error])
          return
        }

        const fileRealPath = archive.realpath(filePath)
        if (fileRealPath === false) {
          const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
          nextTick(callback, [error])
          return
        }

        realpath(asarPath, options, (error, archiveRealPath) => {
          if (error === null) {
            const fullPath = path.join(archiveRealPath, fileRealPath)
            callback(null, fullPath)
          } else {
            callback(error)
          }
        })
      }
    }

    const { realpath } = fs
    fs.realpath = wrapRealpath(realpath)
    fs.realpath.native = wrapRealpath(realpath.native)

    fs.promises.realpath = util.promisify(fs.realpath.native)

    const { exists } = fs
    fs.exists = (pathArgument, callback) => {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return exists(pathArgument, callback)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        nextTick(callback, [error])
        return
      }

      const pathExists = (archive.stat(filePath) !== false)
      nextTick(callback, [pathExists])
    }

    fs.exists[util.promisify.custom] = pathArgument => {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return exists[util.promisify.custom](pathArgument)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        return Promise.reject(error)
      }

      return Promise.resolve(archive.stat(filePath) !== false)
    }

    const { existsSync } = fs
    fs.existsSync = pathArgument => {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return existsSync(pathArgument)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return false

      return archive.stat(filePath) !== false
    }

    const { access } = fs
    fs.access = function (pathArgument, mode, callback) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return access.apply(this, arguments)

      if (typeof mode === 'function') {
        callback = mode
        mode = fs.constants.F_OK
      }

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        nextTick(callback, [error])
        return
      }

      const info = archive.getFileInfo(filePath)
      if (!info) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.access(realPath, mode, callback)
      }

      const stats = archive.stat(filePath)
      if (!stats) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      if (mode & fs.constants.W_OK) {
        const error = createError(AsarError.NO_ACCESS, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      nextTick(callback)
    }

    fs.promises.access = util.promisify(fs.access)

    const { accessSync } = fs
    fs.accessSync = function (pathArgument, mode) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return accessSync.apply(this, arguments)

      if (mode == null) mode = fs.constants.F_OK

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        throw createError(AsarError.INVALID_ARCHIVE, { asarPath })
      }

      const info = archive.getFileInfo(filePath)
      if (!info) {
        throw createError(AsarError.NOT_FOUND, { asarPath, filePath })
      }

      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.accessSync(realPath, mode)
      }

      const stats = archive.stat(filePath)
      if (!stats) {
        throw createError(AsarError.NOT_FOUND, { asarPath, filePath })
      }

      if (mode & fs.constants.W_OK) {
        throw createError(AsarError.NO_ACCESS, { asarPath, filePath })
      }
    }

    const { readFile } = fs
    fs.readFile = function (pathArgument, options, callback) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
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

      const { encoding } = options
      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        nextTick(callback, [error])
        return
      }

      const info = archive.getFileInfo(filePath)
      if (!info) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      if (info.size === 0) {
        nextTick(callback, [null, encoding ? '' : Buffer.alloc(0)])
        return
      }

      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.readFile(realPath, options, callback)
      }

      const buffer = Buffer.alloc(info.size)
      const fd = archive.getFd()
      if (!(fd >= 0)) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      logASARAccess(asarPath, filePath, info.offset)
      fs.read(fd, buffer, 0, info.size, info.offset, error => {
        callback(error, encoding ? buffer.toString(encoding) : buffer)
      })
    }

    fs.promises.readFile = util.promisify(fs.readFile)

    const { readFileSync } = fs
    fs.readFileSync = function (pathArgument, options) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return readFileSync.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) throw createError(AsarError.INVALID_ARCHIVE, { asarPath })

      const info = archive.getFileInfo(filePath)
      if (!info) throw createError(AsarError.NOT_FOUND, { asarPath, filePath })

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

      const { encoding } = options
      const buffer = Buffer.alloc(info.size)
      const fd = archive.getFd()
      if (!(fd >= 0)) throw createError(AsarError.NOT_FOUND, { asarPath, filePath })

      logASARAccess(asarPath, filePath, info.offset)
      fs.readSync(fd, buffer, 0, info.size, info.offset)
      return (encoding) ? buffer.toString(encoding) : buffer
    }

    const { readdir } = fs
    fs.readdir = function (pathArgument, options, callback) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (typeof options === 'function') {
        callback = options
        options = {}
      }
      if (!isAsar) return readdir.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath })
        nextTick(callback, [error])
        return
      }

      const files = archive.readdir(filePath)
      if (!files) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath })
        nextTick(callback, [error])
        return
      }

      nextTick(callback, [null, files])
    }

    fs.promises.readdir = util.promisify(fs.readdir)

    const { readdirSync } = fs
    fs.readdirSync = function (pathArgument, options) {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return readdirSync.apply(this, arguments)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) {
        throw createError(AsarError.INVALID_ARCHIVE, { asarPath })
      }

      const files = archive.readdir(filePath)
      if (!files) {
        throw createError(AsarError.NOT_FOUND, { asarPath, filePath })
      }

      return files
    }

    const { internalModuleReadJSON } = internalBinding('fs')
    internalBinding('fs').internalModuleReadJSON = pathArgument => {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return internalModuleReadJSON(pathArgument)

      const archive = getOrCreateArchive(asarPath)
      if (!archive) return

      const info = archive.getFileInfo(filePath)
      if (!info) return
      if (info.size === 0) return ''
      if (info.unpacked) {
        const realPath = archive.copyFileOut(filePath)
        return fs.readFileSync(realPath, { encoding: 'utf8' })
      }

      const buffer = Buffer.alloc(info.size)
      const fd = archive.getFd()
      if (!(fd >= 0)) return

      logASARAccess(asarPath, filePath, info.offset)
      fs.readSync(fd, buffer, 0, info.size, info.offset)
      return buffer.toString('utf8')
    }

    const { internalModuleStat } = internalBinding('fs')
    internalBinding('fs').internalModuleStat = pathArgument => {
      const { isAsar, asarPath, filePath } = splitPath(pathArgument)
      if (!isAsar) return internalModuleStat(pathArgument)

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
    if (process.platform === 'win32') {
      const { mkdir } = fs
      fs.mkdir = (pathArgument, options, callback) => {
        if (typeof options === 'function') {
          callback = options
          options = {}
        }

        const { isAsar, filePath } = splitPath(pathArgument)
        if (isAsar && filePath.length > 0) {
          const error = createError(AsarError.NOT_DIR)
          nextTick(callback, [error])
          return
        }

        mkdir(pathArgument, options, callback)
      }

      fs.promises.mkdir = util.promisify(fs.mkdir)

      const { mkdirSync } = fs
      fs.mkdirSync = function (pathArgument, options) {
        const { isAsar, filePath } = splitPath(pathArgument)
        if (isAsar && filePath.length) throw createError(AsarError.NOT_DIR)
        return mkdirSync(pathArgument, options)
      }
    }

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

    // Strictly implementing the flags of fs.copyFile is hard, just do a simple
    // implementation for now. Doing 2 copies won't spend much time more as OS
    // has filesystem caching.
    overrideAPI(fs, 'copyFile')
    overrideAPISync(fs, 'copyFileSync')

    overrideAPI(fs, 'open')
    overrideAPISync(process, 'dlopen', 1)
    overrideAPISync(Module._extensions, '.node', 1)
    overrideAPISync(fs, 'openSync')

    const overrideChildProcess = (childProcess) => {
      // Executing a command string containing a path to an asar archive
      // confuses `childProcess.execFile`, which is internally called by
      // `childProcess.{exec,execSync}`, causing Electron to consider the full
      // command as a single path to an archive.
      const { exec, execSync } = childProcess
      childProcess.exec = invokeWithNoAsar(exec)
      childProcess.exec[util.promisify.custom] = invokeWithNoAsar(exec[util.promisify.custom])
      childProcess.execSync = invokeWithNoAsar(execSync)

      overrideAPI(childProcess, 'execFile')
      overrideAPISync(childProcess, 'execFileSync')
    }

    // Lazily override the child_process APIs only when child_process is
    // fetched the first time.  We will eagerly override the child_process APIs
    // when this env var is set so that stack traces generated inside node unit
    // tests will match. This env var will only slow things down in users apps
    // and should not be used.
    if (process.env.ELECTRON_EAGER_ASAR_HOOK_FOR_TESTING) {
      overrideChildProcess(require('child_process'))
    } else {
      const originalModuleLoad = Module._load
      Module._load = (request, ...args) => {
        const loadResult = originalModuleLoad(request, ...args)
        if (request === 'child_process') {
          if (!v8Util.getHiddenValue(loadResult, 'asar-ready')) {
            v8Util.setHiddenValue(loadResult, 'asar-ready', true)
            // Just to make it obvious what we are dealing with here
            const childProcess = loadResult

            overrideChildProcess(childProcess)
          }
        }
        return loadResult
      }
    }
  }
})()
