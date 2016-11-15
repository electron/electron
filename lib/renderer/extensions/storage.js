const getStorage = (storageType) => {
  const data = window.localStorage.getItem(`__chrome.storage.${storageType}__`)
  if (data != null) {
    return JSON.parse(data)
  } else {
    return {}
  }
}

const setStorage = (storageType, storage) => {
  const json = JSON.stringify(storage)
  window.localStorage.setItem(`__chrome.storage.${storageType}__`, json)
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
