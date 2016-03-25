// Deprecate a method.
const deprecate = function (oldName, newName, fn) {
  var warned
  warned = false
  return function () {
    if (!(warned || process.noDeprecation)) {
      warned = true
      deprecate.warn(oldName, newName)
    }
    return fn.apply(this, arguments)
  }
}

// The method is renamed.
deprecate.rename = function (object, oldName, newName) {
  var newMethod, warned
  warned = false
  newMethod = function () {
    if (!(warned || process.noDeprecation)) {
      warned = true
      deprecate.warn(oldName, newName)
    }
    return this[newName].apply(this, arguments)
  }
  if (typeof object === 'function') {
    return object.prototype[oldName] = newMethod
  } else {
    return object[oldName] = newMethod
  }
}

// Forward the method to member.
deprecate.member = function (object, method, member) {
  var warned
  warned = false
  return object.prototype[method] = function () {
    if (!(warned || process.noDeprecation)) {
      warned = true
      deprecate.warn(method, member + '.' + method)
    }
    return this[member][method].apply(this[member], arguments)
  }
}

// Deprecate a property.
deprecate.property = function (object, property, method) {
  return Object.defineProperty(object, property, {
    get: function () {
      var warned
      warned = false
      if (!(warned || process.noDeprecation)) {
        warned = true
        deprecate.warn(property + ' property', method + ' method')
      }
      return this[method]()
    }
  })
}

// Deprecate an event.
deprecate.event = function (emitter, oldName, newName, fn) {
  var warned
  warned = false
  return emitter.on(newName, function (...args) {
    // there is listeners for old API.
    if (this.listenerCount(oldName) > 0) {
      if (!(warned || process.noDeprecation)) {
        warned = true
        deprecate.warn("'" + oldName + "' event", "'" + newName + "' event")
      }
      if (fn != null) {
        return fn.apply(this, arguments)
      } else {
        return this.emit.apply(this, [oldName].concat(args))
      }
    }
  })
}

// Print deprecation warning.
deprecate.warn = function (oldName, newName) {
  return deprecate.log(oldName + ' is deprecated. Use ' + newName + ' instead.')
}

var deprecationHandler = null

// Print deprecation message.
deprecate.log = function (message) {
  if (typeof deprecationHandler === 'function') {
    deprecationHandler(message)
  } else if (process.throwDeprecation) {
    throw new Error(message)
  } else if (process.traceDeprecation) {
    return console.trace(message)
  } else {
    return console.warn('(electron) ' + message)
  }
}

deprecate.setHandler = function (handler) {
  deprecationHandler = handler
}

deprecate.getHandler = function () {
  return deprecationHandler
}

module.exports = deprecate
