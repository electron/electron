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

const getStorageManager = (storageType) => {
  return {
    get (keys, callback) {
      const storage = getStorage(storageType)
      if (keys == null) return storage

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
      if (keys.length === 0) return {}

      let items = {}
      keys.forEach(function (key) {
        var value = storage[key]
        if (value == null) value = defaults[key]
        items[key] = value
      })

      setTimeout(function () {
        callback(items)
      })
    },

    set (items, callback) {
      const storage = getStorage(storageType)

      Object.keys(items).forEach(function (name) {
        storage[name] = items[name]
      })

      setStorage(storageType, storage)

      setTimeout(callback)
    }
  }
}

module.exports = {
  sync: getStorageManager('sync'),
  local: getStorageManager('local')
}
