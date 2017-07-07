const fs = require('fs')
const path = require('path')
const { remote } = require('electron')
const { app } = remote;

const getChromeStoragePath = (storageType) => {
  return path.join(
    app.getPath('userData'), `/Chrome Storage/${storageType}.json`)
}
const readChromeStorageFile = (storageType) => {
  const filePath = getChromeStoragePath(storageType)
  if(!fs.existsSync(filePath)) return null
  return fs.readFileSync(filePath, 'utf8')
}

const writeChromeStorageFile = (storageType, data) => {
  const filePath = getChromeStoragePath(storageType)
  try {
    fs.mkdirSync(path.dirname(filePath))
  } catch (error) {
    // Ignore error
  }
  return fs.writeFileSync(filePath, data)
}

const getStorage = (storageType) => {
  const data = readChromeStorageFile(storageType)
  if (data != null) {
    return JSON.parse(data)
  } else {
    return {}
  }
}

const setStorage = (storageType, storage) => {
  const json = JSON.stringify(storage)
  const data = writeChromeStorageFile(storageType, json)
}

const scheduleCallback = (items, callback) => {
  setTimeout(function () {
    callback(items)
  })
}

const getStorageManager = (storageType) => {
  return {
    get (keys, callback) {
      const storage = getStorage(storageType)
      if (keys == null) return scheduleCallback(storage, callback)

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
      if (keys.length === 0) return scheduleCallback({}, callback)

      let items = {}
      keys.forEach(function (key) {
        var value = storage[key]
        if (value == null) value = defaults[key]
        items[key] = value
      })
      scheduleCallback(items, callback)
    },

    set (items, callback) {
      const storage = getStorage(storageType)

      Object.keys(items).forEach(function (name) {
        storage[name] = items[name]
      })

      setStorage(storageType, storage)

      setTimeout(callback)
    },

    remove (keys, callback) {
      const storage = getStorage(storageType)

      if (!Array.isArray(keys)) {
        keys = [keys]
      }
      keys.forEach(function (key) {
        delete storage[key]
      })

      setStorage(storageType, storage)

      setTimeout(callback)
    },

    clear (callback) {
      setStorage(storageType, {})

      setTimeout(callback)
    }
  }
}

module.exports = {
  sync: getStorageManager('sync'),
  local: getStorageManager('local')
}
