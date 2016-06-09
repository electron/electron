
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

      if (keys === {} || keys === []) return {}
      if (keys === null) return storage

      if (typeof keys === 'string') keys = [keys]

      let items = {}
      // ['foo'] or ['foo', 'bar'] or [{foo: 'bar'}]
      keys.forEach(function (key) {
        if (typeof key === 'string') {
          items[key] = storage[key]
        }
        else {
          const objKey = Object.keys(key)
          if (!storage[objKey] || storage[objKey] === '') {
            items[objKey] = key[objKey]
          } else {
            items[objKey] = storage[objKey]
          }
        }
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
