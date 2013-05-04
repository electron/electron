ipc = require 'ipc'
v8_util = process.atomBinding 'v8_util'

currentContextExist = true

class CallbacksRegistry
  @nextId = 0
  @callbacks = {}

  @add: (callback) ->
    @callbacks[++@nextId] = callback
    @nextId

  @call: (id, args) ->
    @callbacks[id].apply global, args

  @remove: (id) ->
    delete @callbacks[id]

# Convert the arguments object into an array of meta data.
wrapArgs = (args) ->
  Array::slice.call(args).map (value) ->
    if typeof value is 'object' and v8_util.getHiddenValue value, 'isRemoteObject'
      type: 'object', id: value.id
    else if typeof value is 'function'
      type: 'function', id: CallbacksRegistry.add(value)
    else
      type: 'value', value: value

# Convert meta data from browser into real value.
metaToValue = (meta) ->
  switch meta.type
    when 'error' then throw new Error(meta.value)
    when 'value' then meta.value
    when 'array' then (metaToValue(el) for el in meta.members)
    else
      if meta.type is 'function'
        # A shadow class to represent the remote function object.
        ret =
        class RemoteFunction
          constructor: ->
            if @constructor == RemoteFunction
              # Constructor call.
              obj = ipc.sendChannelSync 'ATOM_BROWSER_CONSTRUCTOR', meta.id, wrapArgs(arguments)

              # Returning object in constructor will replace constructed object
              # with the returned object.
              # http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
              return metaToValue obj
            else
              # Function call.
              ret = ipc.sendChannelSync 'ATOM_BROWSER_FUNCTION_CALL', meta.id, wrapArgs(arguments)
              return metaToValue ret
      else
        ret = v8_util.createObjectWithName meta.name

      # Polulate delegate members.
      for member in meta.members
        do (member) ->
          if member.type is 'function'
            ret[member.name] = ->
              # Call member function.
              ret = ipc.sendChannelSync 'ATOM_BROWSER_MEMBER_CALL', meta.id, member.name, wrapArgs(arguments)
              metaToValue ret
          else
            ret.__defineSetter__ member.name, (value) ->
              # Set member data.
              ipc.sendChannelSync 'ATOM_BROWSER_MEMBER_SET', meta.id, member.name, value

            ret.__defineGetter__ member.name, ->
              # Get member data.
              ret = ipc.sendChannelSync 'ATOM_BROWSER_MEMBER_GET', meta.id, member.name
              metaToValue ret

      # Track delegate object's life time, and tell the browser to clean up
      # when the object is GCed.
      v8_util.setDestructor ret, ->
        return unless currentContextExist
        ipc.sendChannel 'ATOM_BROWSER_DEREFERENCE', meta.storeId

      # Mark this is a remote object.
      v8_util.setHiddenValue ret, 'isRemoteObject', true

      ret

# Browser calls a callback in renderer.
ipc.on 'ATOM_RENDERER_CALLBACK', (id, args) ->
  CallbacksRegistry.call id, metaToValue(args)

# A callback in browser is released.
ipc.on 'ATOM_RENDERER_RELEASE_CALLBACK', (id) ->
  CallbacksRegistry.remove id

# Release all resources of current render view when it's going to be unloaded.
window.addEventListener 'unload', (event) ->
  currentContextExist = false
  ipc.sendChannelSync 'ATOM_BROWSER_RELEASE_RENDER_VIEW'

# Get remote module.
exports.require = (module) ->
  meta = ipc.sendChannelSync 'ATOM_BROWSER_REQUIRE', module
  metaToValue meta

# Get object with specified id.
exports.getObject = (id) ->
  meta = ipc.sendChannelSync 'ATOM_BROWSER_REFERENCE', id
  metaToValue meta

# Get current window object.
windowCache = null
exports.getCurrentWindow = ->
  return windowCache if windowCache?
  meta = ipc.sendChannelSync 'ATOM_BROWSER_CURRENT_WINDOW'
  windowCache = metaToValue meta

# Get a global object in browser.
exports.getGlobal = (name) ->
  meta = ipc.sendChannelSync 'ATOM_BROWSER_GLOBAL', name
  metaToValue meta

# Get the process object in browser.
processCache = null
exports.__defineGetter__ 'process', ->
  processCache = exports.getGlobal('process') unless processCache?
  processCache
