const fs = require('fs')
const path = require('path')
const { remote } = require('electron')
const { app } = remote;

const getChromeStoragePath = (storageType, extensionId) => {
  return path.join(
    app.getPath('userData'), `/Chrome Storage/${extensionId}-${storageType}.json`)
}
const readChromeStorageFile = (storageType, extensionId) => {
  const filePath = getChromeStoragePath(storageType, extensionId)
  if(!fs.existsSync(filePath)) return null
  return fs.readFileSync(filePath, 'utf8')
}

const writeChromeStorageFile = (storageType, extensionId, data) => {
  const filePath = getChromeStoragePath(storageType, extensionId)
  try {
    fs.mkdirSync(path.dirname(filePath))
  } catch (error) {
    // Ignore error
  }
  return fs.writeFileSync(filePath, data)
}

const getStorage = (storageType, extensionId) => {
  const data = readChromeStorageFile(storageType, extensionId)
  if (data != null) {
    return JSON.parse(data)
  } else {
    return {}
  }
}

const setStorage = (storageType, extensionId, storage) => {
  const json = JSON.stringify(storage)
  const data = writeChromeStorageFile(storageType, extensionId, json)
}

const scheduleCallback = (items, callback) => {
  setTimeout(function () {
    callback(items)
  })
}

const getStorageManager = (storageType, extensionId) => {
  return {
    get (keys, callback) {
      const storage = getStorage(storageType, extensionId)
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
      const storage = getStorage(storageType, extensionId)

      Object.keys(items).forEach(function (name) {
        storage[name] = items[name]
      })

      setStorage(storageType, extensionId, storage)

      setTimeout(callback)
    },

    remove (keys, callback) {
      const storage = getStorage(storageType, extensionId)

      if (!Array.isArray(keys)) {
        keys = [keys]
      }
      keys.forEach(function (key) {
        delete storage[key]
      })

      setStorage(storageType, extensionId, storage)

      setTimeout(callback)
    },

    clear (callback) {
      setStorage(storageType, extensionId, {})

      setTimeout(callback)
    }
  }
}

module.exports = {
  setup: extensionId => ({
    sync: getStorageManager('sync', extensionId),
    local: getStorageManager('local', extensionId)
  })
}
