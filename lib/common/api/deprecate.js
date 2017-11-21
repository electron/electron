// Deprecate a method.
const deprecate = function (oldName, newName, fn) {
  let warned = false
  return function () {
    if (!(warned || process.noDeprecation)) {
      warned = true
      deprecate.warn(oldName, newName)
    }
    return fn.apply(this, arguments)
  }
}

// The method is aliases and the old method is retained for backwards compat
deprecate.alias = function (object, deprecatedName, existingName) {
  let warned = false
  const newMethod = function () {
    if (!(warned || process.noDeprecation)) {
      warned = true
      deprecate.warn(deprecatedName, existingName)
    }
    return this[existingName].apply(this, arguments)
  }
  if (typeof object === 'function') {
    object.prototype[deprecatedName] = newMethod
  } else {
    object[deprecatedName] = newMethod
  }
}

deprecate.warn = (oldName, newName) => {
  return deprecate.log(`'${oldName}' is deprecated. Use '${newName}' instead.`)
}

let deprecationHandler = null

// Print deprecation message.
deprecate.log = (message) => {
  if (typeof deprecationHandler === 'function') {
    deprecationHandler(message)
  } else if (process.throwDeprecation) {
    throw new Error(message)
  } else if (process.traceDeprecation) {
    return console.trace(message)
  } else {
    return console.warn(`(electron) ${message}`)
  }
}

deprecate.setHandler = (handler) => {
  deprecationHandler = handler
}

deprecate.getHandler = () => deprecationHandler

// None of the below methods are used, and so will be commented
// out until such time that they are needed to be used and tested.

// // Forward the method to member.
// deprecate.member = (object, method, member) => {
//   let warned = false
//   object.prototype[method] = function () {
//     if (!(warned || process.noDeprecation)) {
//       warned = true
//       deprecate.warn(method, `${member}.${method}`)
//     }
//     return this[member][method].apply(this[member], arguments)
//   }
// }
//
// // Deprecate a property.
// deprecate.property = (object, property, method) => {
//   return Object.defineProperty(object, property, {
//     get: function () {
//       let warned = false
//       if (!(warned || process.noDeprecation)) {
//         warned = true
//         deprecate.warn(`${property} property`, `${method} method`)
//       }
//       return this[method]()
//     }
//   })
// }
//
// // Deprecate an event.
// deprecate.event = (emitter, oldName, newName, fn) => {
//   let warned = false
//   return emitter.on(newName, function (...args) {
//     if (this.listenerCount(oldName) > 0) {
//       if (!(warned || process.noDeprecation)) {
//         warned = true
//         deprecate.warn(`'${oldName}' event`, `'${newName}' event`)
//       }
//       if (fn != null) {
//         fn.apply(this, arguments)
//       } else {
//         this.emit.apply(this, [oldName].concat(args))
//       }
//     }
//   })
// }

module.exports = deprecate
