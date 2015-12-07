{ipcRenderer, CallbacksRegistry} = require 'electron'
v8Util = process.atomBinding 'v8_util'

callbacksRegistry = new CallbacksRegistry

# Check for circular reference.
isCircular = (field, visited) ->
  if typeof field is 'object'
    if field in visited
      return true
    visited.push field
  return false

# Convert the arguments object into an array of meta data.
wrapArgs = (args, visited=[]) ->
  valueToMeta = (value) ->
    if Array.isArray value
      type: 'array', value: wrapArgs(value, visited)
    else if Buffer.isBuffer value
      type: 'buffer', value: Array::slice.call(value, 0)
    else if value?.constructor.name is 'Promise'
      type: 'promise', then: valueToMeta(value.then.bind(value))
    else if value? and typeof value is 'object' and v8Util.getHiddenValue value, 'atomId'
      type: 'remote-object', id: v8Util.getHiddenValue value, 'atomId'
    else if value? and typeof value is 'object'
      ret = type: 'object', name: value.constructor.name, members: []
      for prop, field of value
        ret.members.push
          name: prop
          value: valueToMeta(if isCircular(field, visited) then null else field)
      ret
    else if typeof value is 'function' and v8Util.getHiddenValue value, 'returnValue'
      type: 'function-with-return-value', value: valueToMeta(value())
    else if typeof value is 'function'
      type: 'function', id: callbacksRegistry.add(value), location: v8Util.getHiddenValue value, 'location'
    else
      type: 'value', value: value

  Array::slice.call(args).map valueToMeta

# Convert meta data from browser into real value.
metaToValue = (meta) ->
  switch meta.type
    when 'value' then meta.value
    when 'array' then (metaToValue(el) for el in meta.members)
    when 'buffer' then new Buffer(meta.value)
    when 'promise' then Promise.resolve(then: metaToValue(meta.then))
    when 'error' then metaToPlainObject meta
    when 'date' then new Date(meta.value)
    when 'exception'
      throw new Error("#{meta.message}\n#{meta.stack}")
    else
      if meta.type is 'function'
        # A shadow class to represent the remote function object.
        ret =
        class RemoteFunction
          constructor: ->
            if @constructor == RemoteFunction
              # Constructor call.
              obj = ipcRenderer.sendSync 'ATOM_BROWSER_CONSTRUCTOR', meta.id, wrapArgs(arguments)

              # Returning object in constructor will replace constructed object
              # with the returned object.
              # http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
              return metaToValue obj
            else
              # Function call.
              ret = ipcRenderer.sendSync 'ATOM_BROWSER_FUNCTION_CALL', meta.id, wrapArgs(arguments)
              return metaToValue ret
      else
        ret = v8Util.createObjectWithName meta.name

      # Polulate delegate members.
      for member in meta.members
        do (member) ->
          if member.type is 'function'
            ret[member.name] =
            class RemoteMemberFunction
              constructor: ->
                if @constructor is RemoteMemberFunction
                  # Constructor call.
                  obj = ipcRenderer.sendSync 'ATOM_BROWSER_MEMBER_CONSTRUCTOR', meta.id, member.name, wrapArgs(arguments)
                  return metaToValue obj
                else
                  # Call member function.
                  ret = ipcRenderer.sendSync 'ATOM_BROWSER_MEMBER_CALL', meta.id, member.name, wrapArgs(arguments)
                  return metaToValue ret
          else
            Object.defineProperty ret, member.name,
              enumerable: true,
              configurable: false,
              set: (value) ->
                # Set member data.
                ipcRenderer.sendSync 'ATOM_BROWSER_MEMBER_SET', meta.id, member.name, value
                value

              get: ->
                # Get member data.
                ret = ipcRenderer.sendSync 'ATOM_BROWSER_MEMBER_GET', meta.id, member.name
                metaToValue ret

      # Track delegate object's life time, and tell the browser to clean up
      # when the object is GCed.
      v8Util.setDestructor ret, ->
        ipcRenderer.send 'ATOM_BROWSER_DEREFERENCE', meta.id

      # Remember object's id.
      v8Util.setHiddenValue ret, 'atomId', meta.id

      ret

# Construct a plain object from the meta.
metaToPlainObject = (meta) ->
  obj = switch meta.type
    when 'error' then new Error
    else {}
  obj[name] = value for {name, value} in meta.members
  obj

# Browser calls a callback in renderer.
ipcRenderer.on 'ATOM_RENDERER_CALLBACK', (event, id, args) ->
  # Delay the callback to next tick in case the browser is still in the middle
  # of sending this message while the callback sends a sync message to browser,
  # which can fail sometimes.
  setImmediate ->
    callbacksRegistry.apply id, metaToValue(args)

# A callback in browser is released.
ipcRenderer.on 'ATOM_RENDERER_RELEASE_CALLBACK', (event, id) ->
  callbacksRegistry.remove id

# List all built-in modules in browser process.
browserModules = ipcRenderer.sendSync 'ATOM_BROWSER_LIST_MODULES'
# And add a helper receiver for each one.
for name in browserModules
  do (name) ->
    Object.defineProperty exports, name, get: -> exports.getBuiltin name

# Get remote module.
# (Just like node's require, the modules are cached permanently, note that this
#  is safe leak since the object is not expected to get freed in browser)
moduleCache = {}
exports.require = (module) ->
  return moduleCache[module] if moduleCache[module]?

  meta = ipcRenderer.sendSync 'ATOM_BROWSER_REQUIRE', module
  moduleCache[module] = metaToValue meta

# Optimize require('electron').
moduleCache.electron = exports

# Alias to remote.require('electron').xxx.
builtinCache = {}
exports.getBuiltin = (module) ->
  return builtinCache[module] if builtinCache[module]?

  meta = ipcRenderer.sendSync 'ATOM_BROWSER_GET_BUILTIN', module
  builtinCache[module] = metaToValue meta

# Get current BrowserWindow object.
windowCache = null
exports.getCurrentWindow = ->
  return windowCache if windowCache?
  meta = ipcRenderer.sendSync 'ATOM_BROWSER_CURRENT_WINDOW'
  windowCache = metaToValue meta

# Get current WebContents object.
webContentsCache = null
exports.getCurrentWebContents = ->
  return webContentsCache if webContentsCache?
  meta = ipcRenderer.sendSync 'ATOM_BROWSER_CURRENT_WEB_CONTENTS'
  webContentsCache = metaToValue meta

# Get a global object in browser.
exports.getGlobal = (name) ->
  meta = ipcRenderer.sendSync 'ATOM_BROWSER_GLOBAL', name
  metaToValue meta

# Get the process object in browser.
processCache = null
exports.__defineGetter__ 'process', ->
  processCache = exports.getGlobal('process') unless processCache?
  processCache

# Create a funtion that will return the specifed value when called in browser.
exports.createFunctionWithReturnValue = (returnValue) ->
  func = -> returnValue
  v8Util.setHiddenValue func, 'returnValue', true
  func

# Get the guest WebContents from guestInstanceId.
exports.getGuestWebContents = (guestInstanceId) ->
  meta = ipcRenderer.sendSync 'ATOM_BROWSER_GUEST_WEB_CONTENTS', guestInstanceId
  metaToValue meta
