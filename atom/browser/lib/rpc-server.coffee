ipc = require 'ipc'
path = require 'path'
objectsRegistry = require './objects-registry.js'
v8Util = process.atomBinding 'v8_util'

# Convert a real value into meta data.
valueToMeta = (sender, value) ->
  meta = type: typeof value

  meta.type = 'buffer' if Buffer.isBuffer value
  meta.type = 'value' if value is null
  meta.type = 'array' if Array.isArray value

  # Treat the arguments object as array.
  meta.type = 'array' if meta.type is 'object' and value.callee? and value.length?

  if meta.type is 'array'
    meta.members = []
    meta.members.push valueToMeta(sender, el) for el in value
  else if meta.type is 'object' or meta.type is 'function'
    meta.name = value.constructor.name

    # Reference the original value if it's an object, because when it's
    # passed to renderer we would assume the renderer keeps a reference of
    # it.
    [meta.id, meta.storeId] = objectsRegistry.add sender.getId(), value

    meta.members = []
    meta.members.push {name: prop, type: typeof field} for prop, field of value
  else if meta.type is 'buffer'
    meta.value = Array::slice.call value, 0
  else
    meta.type = 'value'
    meta.value = value

  meta

# Convert Error into meta data.
errorToMeta = (error) ->
  type: 'error', message: error.message, stack: (error.stack || error)

# Convert array of meta data from renderer into array of real values.
unwrapArgs = (sender, args) ->
  metaToValue = (meta) ->
    switch meta.type
      when 'value' then meta.value
      when 'remote-object' then objectsRegistry.get meta.id
      when 'array' then unwrapArgs sender, meta.value
      when 'buffer' then new Buffer(meta.value)
      when 'object'
        ret = v8Util.createObjectWithName meta.name
        for member in meta.members
          ret[member.name] = metaToValue(member.value)
        ret
      when 'function-with-return-value'
        returnValue = metaToValue meta.value
        -> returnValue
      when 'function'
        rendererReleased = false
        objectsRegistry.once "clear-#{sender.getId()}", ->
          rendererReleased = true

        ret = ->
          throw new Error('Calling a callback of released renderer view') if rendererReleased
          sender.send 'ATOM_RENDERER_CALLBACK', meta.id, valueToMeta(sender, arguments)
        v8Util.setDestructor ret, ->
          return if rendererReleased
          sender.send 'ATOM_RENDERER_RELEASE_CALLBACK', meta.id
        ret
      else throw new TypeError("Unknown type: #{meta.type}")

  args.map metaToValue

# Call a function and send reply asynchronously if it's a an asynchronous
# style function and the caller didn't pass a callback.
callFunction = (event, func, caller, args) ->
  if v8Util.getHiddenValue(func, 'asynchronous') and typeof args[args.length - 1] isnt 'function'
    args.push (ret) ->
      event.returnValue = valueToMeta event.sender, ret
    func.apply caller, args
  else
    ret = func.apply caller, args
    event.returnValue = valueToMeta event.sender, ret

# Send by BrowserWindow when its render view is deleted.
process.on 'ATOM_BROWSER_RELEASE_RENDER_VIEW', (id) ->
  objectsRegistry.clear id

ipc.on 'ATOM_BROWSER_REQUIRE', (event, module) ->
  try
    event.returnValue = valueToMeta event.sender, process.mainModule.require(module)
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_GLOBAL', (event, name) ->
  try
    event.returnValue = valueToMeta event.sender, global[name]
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_CURRENT_WINDOW', (event, guestInstanceId) ->
  try
    BrowserWindow = require 'browser-window'
    if guestInstanceId?
      guestViewManager = require './guest-view-manager'
      window = BrowserWindow.fromWebContents guestViewManager.getEmbedder(guestInstanceId)
    else
      window = BrowserWindow.fromWebContents event.sender
      window = BrowserWindow.fromDevToolsWebContents event.sender unless window?
    event.returnValue = valueToMeta event.sender, window
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_CURRENT_WEB_CONTENTS', (event) ->
  event.returnValue = valueToMeta event.sender, event.sender

ipc.on 'ATOM_BROWSER_CONSTRUCTOR', (event, id, args) ->
  try
    args = unwrapArgs event.sender, args
    constructor = objectsRegistry.get id
    # Call new with array of arguments.
    # http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    obj = new (Function::bind.apply(constructor, [null].concat(args)))
    event.returnValue = valueToMeta event.sender, obj
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_FUNCTION_CALL', (event, id, args) ->
  try
    args = unwrapArgs event.sender, args
    func = objectsRegistry.get id
    callFunction event, func, global, args
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_CONSTRUCTOR', (event, id, method, args) ->
  try
    args = unwrapArgs event.sender, args
    constructor = objectsRegistry.get(id)[method]
    # Call new with array of arguments.
    obj = new (Function::bind.apply(constructor, [null].concat(args)))
    event.returnValue = valueToMeta event.sender, obj
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_CALL', (event, id, method, args) ->
  try
    args = unwrapArgs event.sender, args
    obj = objectsRegistry.get id
    callFunction event, obj[method], obj, args
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_SET', (event, id, name, value) ->
  try
    obj = objectsRegistry.get id
    obj[name] = value
    event.returnValue = null
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_MEMBER_GET', (event, id, name) ->
  try
    obj = objectsRegistry.get id
    event.returnValue = valueToMeta event.sender, obj[name]
  catch e
    event.returnValue = errorToMeta e

ipc.on 'ATOM_BROWSER_DEREFERENCE', (event, storeId) ->
  objectsRegistry.remove event.sender.getId(), storeId

ipc.on 'ATOM_BROWSER_GUEST_WEB_CONTENTS', (event, guestInstanceId) ->
  try
    guestViewManager = require './guest-view-manager'
    event.returnValue = valueToMeta event.sender, guestViewManager.getGuest(guestInstanceId)
  catch e
    event.returnValue = errorToMeta e
