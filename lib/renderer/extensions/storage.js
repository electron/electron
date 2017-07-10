const fs = require('fs')
const path = require('path')
const { remote } = require('electron')
const { app } = remote

const getChromeStoragePath = (storageType, extensionId) => {
  return path.join(
    app.getPath('userData'), `/Chrome Storage/${extensionId}-${storageType}.json`)
}

// recursively create directory and parent directories if needed
const mkdirParent = (dirPath, mode, callback) => {
  fs.mkdir(dirPath, mode, error => {
    if (error && error.errno === 34) {
      fs.mkdirParent(path.dirname(dirPath), mode, callback)
      fs.mkdirParent(dirPath, mode, callback)
    }
    callback && callback(error)
  })
}

const readChromeStorageFile = (storageType, extensionId, cb) => {
  const filePath = getChromeStoragePath(storageType, extensionId)
  fs.readFile(filePath, 'utf8', (err, data) => {
    if (err && err.code === 'ENOENT') {
      return cb(null, null)
    }
    cb(err, data)
  })
}

const writeChromeStorageFile = (storageType, extensionId, data, cb) => {
  const filePath = getChromeStoragePath(storageType, extensionId)

  mkdirParent(path.dirname(filePath), err => {
    if (err) { /* we just ignore the errors of mkdir or mkdirParent */ }
    fs.writeFile(filePath, data, cb)
  })
}

const getStorage = (storageType, extensionId, cb) => {
  readChromeStorageFile(storageType, extensionId, (err, data) => {
    if (err) throw err
    if (!cb) throw new TypeError('No callback provided')

    if (data !== null) {
      cb(JSON.parse(data))
    } else {
      cb({})
    }
  })
}

const setStorage = (storageType, extensionId, storage, cb) => {
  const json = JSON.stringify(storage)
  writeChromeStorageFile(storageType, extensionId, json, err => {
    if (err) throw err
    if (cb) cb()
  })
}

const getStorageManager = (storageType, extensionId) => {
  return {
    get (keys, callback) {
      getStorage(storageType, extensionId, storage => {
        if (keys == null) return callback(storage)

        let defaults = {}
        switch (typeof keys) {
          case 'string':
            keys = [keys]
            break
          case 'object':
            if (!Array.isArray(keys)) {
              defaults = keys
              keys = Object.keys(keys)
            }
            break
        }
        if (keys.length === 0) return callback({})

        let items = {}
        keys.forEach(function (key) {
          var value = storage[key]
          if (value == null) value = defaults[key]
          items[key] = value
        })
        callback(items)
      })
    },

    set (items, callback) {
      getStorage(storageType, extensionId, storage => {
        Object.keys(items).forEach(function (name) {
          storage[name] = items[name]
        })

        setStorage(storageType, extensionId, storage, callback)
      })
    },

    remove (keys, callback) {
      getStorage(storageType, extensionId, storage => {
        if (!Array.isArray(keys)) {
          keys = [keys]
        }
        keys.forEach(function (key) {
          delete storage[key]
        })

        setStorage(storageType, extensionId, storage, callback)
      })
    },

    clear (callback) {
      setStorage(storageType, extensionId, {}, callback)
    }
  }
}

module.exports = {
  setup: extensionId => ({
    sync: getStorageManager('sync', extensionId),
    local: getStorageManager('local', extensionId)
  })
}
