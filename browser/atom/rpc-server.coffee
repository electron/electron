ipc = require 'ipc'
path = require 'path'
objectsRegistry = require './objects-registry.js'
v8Util = process.atomBinding 'v8_util'

# Convert a real value into meta data.
valueToMeta = (processId, routingId, value) ->
  meta = type: typeof value

  meta.type = 'value' if value is null
  meta.type = 'array' if Array.isArray value

  # Treat the arguments object as array.
  meta.type = 'array' if meta.type is 'object' and value.callee? and value.length?

  if meta.type is 'array'
    meta.members = []
    meta.members.push valueToMeta(processId, routingId, el) for el in value
  else if meta.type is 'object' or meta.type is 'function'
    meta.name = value.constructor.name

    # Reference the original value if it's an object, because when it's
    # passed to renderer we would assume the renderer keeps a reference of
    # it.
    [meta.id, meta.storeId] = objectsRegistry.add processId, routingId, value

    meta.members = []
    meta.members.push {name: prop, type: typeof field} for prop, field of value
  else
    meta.type = 'value'
    meta.value = value

  meta

# Convert Error into meta data.
errorToMeta = (error) ->
  type: 'error', message: error.message, stack: (error.stack || error)

# Convert array of meta data from renderer into array of real values.
unwrapArgs = (processId, routingId, args) ->
  metaToValue = (meta) ->
    switch meta.type
      when 'value' then meta.value
      when 'remote-object' then objectsRegistry.get meta.id
      when 'array' then unwrapArgs processId, routingId, meta.value
      when 'object'
        ret = v8Util.createObjectWithName meta.name
        for member in meta.members
          ret[member.name] = metaToValue(member.value)
        ret
      when 'function-with-return-value'
        returnValue = metaToValue meta.value
        -> returnValue
      when 'function'
        ret = ->
          ipc.sendChannel processId, routingId, 'ATOM_RENDERER_CALLBACK', meta.id, valueToMeta(processId, routingId, arguments)
        v8Util.setDestructor ret, ->
          ipc.sendChannel processId, routingId, 'ATOM_RENDERER_RELEASE_CALLBACK', meta.id
        ret
      else throw new TypeError("Unknown type: #{meta.type}")

  args.map metaToValue

# Call a function and send reply asynchronously if it's a an asynchronous
# style function and the caller didn't pass a callback.
callFunction = (event, processId, routingId, func, caller, args) ->
  if v8Util.getHiddenValue(func, 'asynchronous') and typeof args[args.length - 1] isnt 'function'
    args.push (ret) ->
      event.returnValue = valueToMeta processId, routingId, ret
    func.apply caller, args
  else
    ret = func.apply caller, args
    event.returnValue = valueToMeta processId, routingId, ret

ipc.on 'ATOM_BROWSER_REQUIRE', (event, processId, routingId, module) ->
  try
    event.returnValue = valueToMeta processId, routingId, require(module)
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_GLOBAL', (event, processId, routingId, name) ->
  try
    event.returnValue = valueToMeta processId, routingId, global[name]
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_RELEASE_RENDER_VIEW', (event, processId, routingId) ->
  objectsRegistry.clear processId, routingId
  event.returnValue = null

ipc.on 'ATOM_BROWSER_CURRENT_WINDOW', (event, processId, routingId) ->
  try
    BrowserWindow = require 'browser-window'
    window = BrowserWindow.fromProcessIdAndRoutingId processId, routingId
    event.returnValue = valueToMeta processId, routingId, window
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_CONSTRUCTOR', (event, processId, routingId, id, args) ->
  try
    args = unwrapArgs processId, routingId, args
    constructor = objectsRegistry.get id
    # Call new with array of arguments.
    # http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    obj = new (Function::bind.apply(constructor, [null].concat(args)))
    event.returnValue = valueToMeta processId, routingId, obj
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_FUNCTION_CALL', (event, processId, routingId, id, args) ->
  try
    args = unwrapArgs processId, routingId, args
    func = objectsRegistry.get id
    callFunction event, processId, routingId, func, global, args
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_CONSTRUCTOR', (event, processId, routingId, id, method, args) ->
  try
    args = unwrapArgs processId, routingId, args
    constructor = objectsRegistry.get(id)[method]
    # Call new with array of arguments.
    obj = new (Function::bind.apply(constructor, [null].concat(args)))
    event.returnValue = valueToMeta processId, routingId, obj
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_CALL', (event, processId, routingId, id, method, args) ->
  try
    args = unwrapArgs processId, routingId, args
    obj = objectsRegistry.get id
    callFunction event, processId, routingId, obj[method], obj, args
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_SET', (event, processId, routingId, id, name, value) ->
  try
    obj = objectsRegistry.get id
    obj[name] = value
    event.returnValue = null
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_GET', (event, processId, routingId, id, name) ->
  try
    obj = objectsRegistry.get id
    event.returnValue = valueToMeta processId, routingId, obj[name]
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_DEREFERENCE', (processId, routingId, storeId) ->
  objectsRegistry.remove processId, routingId, storeId
