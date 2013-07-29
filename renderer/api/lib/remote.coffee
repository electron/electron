ipc = require 'ipc'
CallbacksRegistry = require 'callbacks-registry'
v8Util = process.atomBinding 'v8_util'

currentContextExist = true
callbacksRegistry = new CallbacksRegistry

# Convert the arguments object into an array of meta data.
wrapArgs = (args) ->
  Array::slice.call(args).map (value) ->
    if value? and typeof value is 'object' and v8Util.getHiddenValue value, 'atomId'
      type: 'object', id: v8Util.getHiddenValue value, 'atomId'
    else if typeof value is 'function'
      type: 'function', id: callbacksRegistry.add(value)
    else
      type: 'value', value: value

# Convert meta data from browser into real value.
metaToValue = (meta) ->
  switch meta.type
    when 'value' then meta.value
    when 'array' then (metaToValue(el) for el in meta.members)
    when 'error'
      console.log meta.stack
      throw new Error(meta.message)
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
        ret = v8Util.createObjectWithName meta.name

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
      v8Util.setDestructor ret, ->
        return unless currentContextExist
        ipc.sendChannel 'ATOM_BROWSER_DEREFERENCE', meta.storeId

      # Remember object's id.
      v8Util.setHiddenValue ret, 'atomId', meta.id

      ret

# Browser calls a callback in renderer.
ipc.on 'ATOM_RENDERER_CALLBACK', (id, args) ->
  callbacksRegistry.apply id, metaToValue(args)

# A callback in browser is released.
ipc.on 'ATOM_RENDERER_RELEASE_CALLBACK', (id) ->
  callbacksRegistry.remove id

# Release all resources of current render view when it's going to be unloaded.
window.addEventListener 'unload', (event) ->
  currentContextExist = false
  ipc.sendChannelSync 'ATOM_BROWSER_RELEASE_RENDER_VIEW'

# Get remote module.
# (Just like node's require, the modules are cached permanently, note that this
#  is safe leak since the object is not expected to get freed in browser)
moduleCache = {}
exports.require = (module) ->
  return moduleCache[module] if moduleCache[module]?

  meta = ipc.sendChannelSync 'ATOM_BROWSER_REQUIRE', module
  moduleCache[module] = metaToValue meta

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
