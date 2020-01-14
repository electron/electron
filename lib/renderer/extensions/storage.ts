import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'

const getStorage = (storageType: string, extensionId: number, callback: Function) => {
  if (typeof callback !== 'function') throw new TypeError('No callback provided')

  ipcRendererInternal.invoke<string>('CHROME_STORAGE_READ', storageType, extensionId)
    .then(data => {
      if (data !== null) {
        callback(JSON.parse(data))
      } else {
        // Disabled due to false positive in StandardJS
        // eslint-disable-next-line standard/no-callback-literal
        callback({})
      }
    })
}

const setStorage = (storageType: string, extensionId: number, storage: Record<string, any>, callback: Function) => {
  const json = JSON.stringify(storage)
  ipcRendererInternal.invoke('CHROME_STORAGE_WRITE', storageType, extensionId, json)
    .then(() => {
      if (callback) callback()
    })
}

const getStorageManager = (storageType: string, extensionId: number) => {
  return {
    get (keys: string[], callback: Function) {
      getStorage(storageType, extensionId, (storage: Record<string, any>) => {
        if (keys == null) return callback(storage)

        let defaults: Record<string, any> = {}
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

        // Disabled due to false positive in StandardJS
        // eslint-disable-next-line standard/no-callback-literal
        if (keys.length === 0) return callback({})

        const items: Record<string, any> = {}
        keys.forEach((key: string) => {
          let value = storage[key]
          if (value == null) value = defaults[key]
          items[key] = value
        })
        callback(items)
      })
    },

    set (items: Record<string, any>, callback: Function) {
      getStorage(storageType, extensionId, (storage: Record<string, any>) => {
        Object.keys(items).forEach(name => { storage[name] = items[name] })
        setStorage(storageType, extensionId, storage, callback)
      })
    },

    remove (keys: string[], callback: Function) {
      getStorage(storageType, extensionId, (storage: Record<string, any>) => {
        if (!Array.isArray(keys)) keys = [keys]
        keys.forEach((key: string) => {
          delete storage[key]
        })

        setStorage(storageType, extensionId, storage, callback)
      })
    },

    clear (callback: Function) {
      setStorage(storageType, extensionId, {}, callback)
    }
  }
}

export const setup = (extensionId: number) => ({
  sync: getStorageManager('sync', extensionId),
  local: getStorageManager('local', extensionId)
})
