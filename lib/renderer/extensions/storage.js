
const getStorage  = () => {
  const data = window.localStorage.getItem('__chrome.storage.sync__')
  if (data != null) {
    return JSON.parse(data)
  } else {
    return {}
  }
}

const setStorage = (storage) => {
  const json = JSON.stringify(storage)
  window.localStorage.setItem('__chrome.storage.sync__', json)
}

module.exports = {
  sync: {
    get (keys, callback) {
      const storage = getStorage()
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
          break;
      }
      if (keys.length === 0 ) return {}

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
      const storage = getStorage()

      Object.keys(items).forEach(function (name) {
        storage[name] = items[name]
      })

      setStorage(storage)

      setTimeout(callback)
    }
  }
}
