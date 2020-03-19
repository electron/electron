let deprecationHandler: ElectronInternal.DeprecationHandler | null = null

function warnOnce (oldName: string, newName?: string) {
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

const deprecate: ElectronInternal.DeprecationUtil = {
  warnOnce,
  setHandler: (handler) => { deprecationHandler = handler },
  getHandler: () => deprecationHandler,
  warn: (oldName, newName) => {
    if (!process.noDeprecation) {
      deprecate.log(`'${oldName}' is deprecated. Use '${newName}' instead.`)
    }
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

  // remove a function with no replacement
  removeFunction: (fn, removedName) => {
    if (!fn) { throw Error(`'${removedName} function' is invalid or does not exist.`) }

    // wrap the deprecated function to warn user
    const warn = warnOnce(`${fn.name} function`)
    return function (this: any) {
      warn()
      fn.apply(this, arguments)
    }
  },

  // change the name of a function
  renameFunction: (fn, newName) => {
    const warn = warnOnce(`${fn.name} function`, `${newName} function`)
    return function (this: any) {
      warn()
      return fn.apply(this, arguments)
    }
  },

  moveAPI: (fn: Function, oldUsage: string, newUsage: string) => {
    const warn = warnOnce(oldUsage, newUsage)
    return function (this: any) {
      warn()
      return fn.apply(this, arguments)
    }
  },

  // change the name of an event
  event: (emitter, oldName, newName) => {
    const warn = newName.startsWith('-') /* internal event */
      ? warnOnce(`${oldName} event`)
      : warnOnce(`${oldName} event`, `${newName} event`)
    return emitter.on(newName, function (this: NodeJS.EventEmitter, ...args) {
      if (this.listenerCount(oldName) !== 0) {
        warn()
        this.emit(oldName, ...args)
      }
    })
  },

  // deprecate a getter/setter function pair in favor of a property
  fnToProperty: (prototype: any, prop: string, getter: string, setter?: string) => {
    const withWarnOnce = function (obj: any, key: any, oldName: string, newName: string) {
      const warn = warnOnce(oldName, newName)
      const method = obj[key]
      return function (this: any, ...args: any) {
        warn()
        return method.apply(this, args)
      }
    }

    prototype[getter.substr(1)] = withWarnOnce(prototype, getter, `${getter.substr(1)} function`, `${prop} property`)
    if (setter) {
      prototype[setter.substr(1)] = withWarnOnce(prototype, setter, `${setter.substr(1)} function`, `${prop} property`)
    }
  },

  // remove a property with no replacement
  removeProperty: (o, removedName, onlyForValues) => {
    // if the property's already been removed, warn about it
    const info = Object.getOwnPropertyDescriptor((o as any).__proto__, removedName) // eslint-disable-line
    if (!info) {
      deprecate.log(`Unable to remove property '${removedName}' from an object that lacks it.`)
      return o
    }
    if (!info.get || !info.set) {
      deprecate.log(`Unable to remove property '${removedName}' from an object does not have a getter / setter`)
      return o
    }

    // wrap the deprecated property in an accessor to warn
    const warn = warnOnce(removedName)
    return Object.defineProperty(o, removedName, {
      configurable: true,
      get: () => {
        warn()
        return info.get!.call(o)
      },
      set: newVal => {
        if (!onlyForValues || onlyForValues.includes(newVal)) {
          warn()
        }
        return info.set!.call(o, newVal)
      }
    })
  },

  // change the name of a property
  renameProperty: (o, oldName, newName) => {
    const warn = warnOnce(oldName, newName)

    // if the new property isn't there yet,
    // inject it and warn about it
    if ((oldName in o) && !(newName in o)) {
      warn()
      o[newName] = (o as any)[oldName]
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

export default deprecate
