ipc = require 'ipc'
IDWeakMap = require 'id_weak_map'
v8_util = process.atomBinding 'v8_util'

class CallbacksRegistry
  constructor: ->
    @referencesMap = {}
    @weakMap = new IDWeakMap

  get: (id) -> @weakMap.get id
  remove: (id) -> delete @referencesMap[id]

  add: (callback) ->
    id = @weakMap.add callback
    @referencesMap[id] = callback
    id

# Translate arguments object into a list of meta data.
# Unlike the Meta class in browser, this function only create delegate object
# for functions, other types of value are transfered after serialization.
callbacksRegistry = new CallbacksRegistry
argumentsToMetaList = (args) ->
  metas = []
  for arg in args
    if typeof arg is 'function'
      metas.push type: 'function', id: callbacksRegistry.add(arg)
    else
      metas.push type: 'value', value: arg
  metas

# Transform the description of value into a value or delegate object.
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
              obj = ipc.sendChannelSync 'ATOM_BROWSER_CONSTRUCTOR', meta.id, argumentsToMetaList(arguments)

              # Returning object in constructor will replace constructed object
              # with the returned object.
              # http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
              return metaToValue obj
            else
              # Function call.
              ret = ipc.sendChannelSync 'ATOM_BROWSER_FUNCTION_CALL', meta.id, argumentsToMetaList(arguments)
              return metaToValue ret
      else
        ret = v8_util.createObjectWithName meta.name

      # Polulate delegate members.
      for member in meta.members
        do (member) ->
          if member.type is 'function'
            ret[member.name] = ->
              # Call member function.
              ret = ipc.sendChannelSync 'ATOM_BROWSER_MEMBER_CALL', meta.id, member.name, argumentsToMetaList(arguments)
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
        ipc.sendChannel 'ATOM_BROWSER_DEREFERENCE', meta.storeId

      ret

# Browser calls a callback in renderer.
ipc.on 'ATOM_RENDERER_FUNCTION_CALL', (callbackId, args) ->
  callback = callbacksRegistry.get callbackId
  callback.apply global, metaToValue(args)

# Browser releases a callback in renderer.
ipc.on 'ATOM_RENDERER_DEREFERENCE', (callbackId) ->
  callbacksRegistry.remove callbackId

# Get remote module.
exports.require = (module) ->
  meta = ipc.sendChannelSync 'ATOM_BROWSER_REQUIRE', module
  metaToValue meta

# Get object with specified id.
exports.getObject = (id) ->
  meta = ipc.sendChannelSync 'ATOM_BROWSER_REFERENCE', id
  metaToValue meta

# Get current window object.
exports.getCurrentWindow = ->
  meta = ipc.sendChannelSync 'ATOM_BROWSER_CURRENT_WINDOW'
  metaToValue meta
