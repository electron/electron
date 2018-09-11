
let deprecationHandler = null

const deprecate = {
  setHandler: (handler) => { deprecationHandler = handler },
  getHandler: () => deprecationHandler,
  warn: (oldName, newName) => {
    return deprecate.log(`'${oldName}' is deprecated. Use '${newName}' instead.`)
  },
  log: (message) => {
    if (typeof deprecationHandler === 'function') {
      deprecationHandler(message)
    } else if (process.throwDeprecation) {
      throw new Error(message)
    } else if (process.traceDeprecation) {
      return console.trace(message)
    } else {
      return console.warn(`(electron) ${message}`)
    }
  },
  renameFunction: function (fn, oldName, newName) {
    let warned = false
    return function () {
      if (!(warned || process.noDeprecation)) {
        warned = true
        deprecate.warn(oldName, newName)
      }
      return fn.apply(this, arguments)
    }
  },
  removeFunction: (oldName) => {
    if (!process.noDeprecation) {
      deprecate.log(`The '${oldName}' function has been deprecated and marked for removal.`)
    }
  },
  alias: function (object, oldName, newName) {
    let warned = false
    const newFn = function () {
      if (!(warned || process.noDeprecation)) {
        warned = true
        deprecate.warn(oldName, newName)
      }
      return this[newName].apply(this, arguments)
    }
    if (typeof object === 'function') {
      object.prototype[newName] = newFn
    } else {
      object[oldName] = newFn
    }
  },
  event: (emitter, oldName, newName) => {
    let warned = false
    return emitter.on(newName, function (...args) {
      if (this.listenerCount(oldName) === 0) return
      if (warned || process.noDeprecation) return

      warned = true
      if (newName.startsWith('-')) {
        deprecate.log(`'${oldName}' event has been deprecated.`)
      } else {
        deprecate.warn(`'${oldName}' event`, `'${newName}' event`)
      }
      this.emit(oldName, ...args)
    })
  },
  removeProperty: (object, deprecated) => {
    let warned = false
    let warn = () => {
      if (!(warned || process.noDeprecation)) {
        warned = true
        deprecate.log(`The '${deprecated}' property has been deprecated and marked for removal.`)
      }
    }

    if (!(deprecated in object)) {
      throw new Error('Cannot deprecate a property on an object which does not have that property')
    }

    let temp = object[deprecated]
    return Object.defineProperty(object, deprecated, {
      configurable: true,
      get: () => {
        warn()
        return temp
      },
      set: (newValue) => {
        warn()
        temp = newValue
      }
    })
  },
  renameProperty: (object, oldName, newName) => {
    let warned = false
    let warn = () => {
      if (!(warned || process.noDeprecation)) {
        warned = true
        deprecate.warn(oldName, newName)
      }
    }

    if (!(newName in object) && (oldName in object)) {
      warn()
      object[newName] = object[oldName]
    }

    return Object.defineProperty(object, oldName, {
      get: function () {
        warn()
        return this[newName]
      },
      set: function (value) {
        warn()
        this[newName] = value
      }
    })
  }
}

module.exports = deprecate
