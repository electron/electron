# Deprecate a method.
deprecate = (oldName, newName, fn) ->
  warned = false
  ->
    unless warned or process.noDeprecation
      warned = true
      deprecate.warn oldName, newName
    fn.apply this, arguments

# The method is renamed.
deprecate.rename = (object, oldName, newName) ->
  warned = false
  newMethod = ->
    unless warned or process.noDeprecation
      warned = true
      deprecate.warn oldName, newName
    this[newName].apply this, arguments
  if typeof object is 'function'
    object.prototype[oldName] = newMethod
  else
    object[oldName] = newMethod

# Forward the method to member.
deprecate.member = (object, method, member) ->
  warned = false
  object.prototype[method] = ->
    unless warned or process.noDeprecation
      warned = true
      deprecate.warn method, "#{member}.#{method}"
    this[member][method].apply this[member], arguments

# Deprecate a property.
deprecate.property = (object, property, method) ->
  Object.defineProperty object, property,
    get: ->
      warned = false
      unless warned or process.noDeprecation
        warned = true
        deprecate.warn "#{property} property", "#{method} method"
      this[method]()

# Deprecate an event.
deprecate.event = (emitter, oldName, newName, fn) ->
  warned = false
  emitter.on newName, ->
    if @listenerCount(oldName) > 0  # there is listeners for old API.
      unless warned or process.noDeprecation
        warned = true
        deprecate.warn "'#{oldName}' event", "'#{newName}' event"
      fn.apply this, arguments

# Print deprecate warning.
deprecate.warn = (oldName, newName) ->
  message = "#{oldName} is deprecated. Use #{newName} instead."
  if process.throwDeprecation
    throw new Error(message)
  else if process.traceDeprecation
    console.trace message
  else
    console.warn "(electron) #{message}"

module.exports = deprecate
