'use strict'

let deprecationHandler = null

function warnOnce (oldName, newName) {
  let warned = false
  const msg = newName
    ? `'${oldName}' is deprecated and will be removed. Please use '${newName}' instead.`
    : `'${oldName}' is deprecated and will be removed.`
  return () => {
    if (!warned && !process.noDeprecation) {
      warned = true
      deprecate.log(msg)
    }
  }
}

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

  event: (emitter, oldName, newName) => {
    const warn = newName.startsWith('-') /* internal event */
      ? warnOnce(`${oldName} event`)
      : warnOnce(`${oldName} event`, `${newName} event`)
    return emitter.on(newName, function (...args) {
      if (this.listenerCount(oldName) !== 0) {
        warn()
        this.emit(oldName, ...args)
      }
    })
  },

  removeProperty: (o, removedName) => {
    // if the property's already been removed, warn about it
    if (!(removedName in o)) {
      deprecate.log(`Unable to remove property '${removedName}' from an object that lacks it.`)
    }

    // wrap the deprecated property in an accessor to warn
    const warn = warnOnce(removedName)
    let val = o[removedName]
    return Object.defineProperty(o, removedName, {
      configurable: true,
      get: () => {
        warn()
        return val
      },
      set: newVal => {
        warn()
        val = newVal
      }
    })
  },

  renameProperty: (o, oldName, newName) => {
    const warn = warnOnce(oldName, newName)

    // if the new property isn't there yet,
    // inject it and warn about it
    if ((oldName in o) && !(newName in o)) {
      warn()
      o[newName] = o[oldName]
    }

    // wrap the deprecated property in an accessor to warn
    // and redirect to the new property
    return Object.defineProperty(o, oldName, {
      get: () => {
        warn()
        return o[newName]
      },
      set: value => {
        warn()
        o[newName] = value
      }
    })
  }
}

module.exports = deprecate
