ipc = require 'ipc'
path = require 'path'
objectsRegistry = require './objects_registry.js'
v8_util = process.atomBinding 'v8_util'

# Convert list of meta information into real arguments array, the main
# purpose is to turn remote function's id into delegate function.
argsToValues = (processId, routingId, metas) ->
  constructCallback = (meta) ->
    return meta.value if meta.type is 'value'

    # Create a delegate function to do asynchronous RPC call.
    ret = ->
      args = new Meta(processId, routingId, arguments)
      ipc.sendChannel processId, routingId, 'ATOM_RENDERER_FUNCTION_CALL', meta.id, args
    v8_util.setDestructor ret, ->
      ipc.sendChannel processId, routingId, 'ATOM_RENDERER_DEREFERENCE', meta.id
    ret

  constructCallback meta for meta in metas

# Convert a real value into a POD structure which carries information of this
# value.
class Meta
  constructor: (processId, routingId, value) ->
    @type = typeof value
    @type = 'value' if value is null
    @type = 'array' if Array.isArray value

    # Treat the arguments object as array.
    @type = 'array' if @type is 'object' and value.callee? and value.length?

    if @type is 'array'
      @members = []
      @members.push new Meta(processId, routingId, el) for el in value
    else if @type is 'object' or @type is 'function'
      @name = value.constructor.name

      # Reference the original value if it's an object, because when it's
      # passed to renderer we would assume the renderer keeps a reference of
      # it.
      @storeId = objectsRegistry.add processId, routingId, value
      @id = value.id

      @members = []
      @members.push { name: prop, type: typeof field } for prop, field of value
    else
      @type = 'value'
      @value = value

ipc.on 'ATOM_BROWSER_REQUIRE', (event, processId, routingId, module) ->
  try
    event.result = new Meta(processId, routingId, require(module))
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_GLOBAL', (event, processId, routingId, name) ->
  try
    event.result = new Meta(processId, routingId, global[name])
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_CURRENT_WINDOW', (event, processId, routingId) ->
  try
    windows = objectsRegistry.getAllWindows()
    for window in windows
      break if window.getProcessID() == processId and
               window.getRoutingID() == routingId
    event.result = new Meta(processId, routingId, window)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_CONSTRUCTOR', (event, processId, routingId, id, args) ->
  try
    args = argsToValues processId, routingId, args
    constructor = objectsRegistry.get id
    # Call new with array of arguments.
    # http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    obj = new (Function::bind.apply(constructor, [null].concat(args)))
    event.result = new Meta(processId, routingId, obj)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_FUNCTION_CALL', (event, processId, routingId, id, args) ->
  try
    args = argsToValues processId, routingId, args
    func = objectsRegistry.get id
    ret = func.apply global, args
    event.result = new Meta(processId, routingId, ret)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_MEMBER_CALL', (event, processId, routingId, id, method, args) ->
  try
    args = argsToValues processId, routingId, args
    obj = objectsRegistry.get id
    ret = obj[method].apply(obj, args)
    event.result = new Meta(processId, routingId, ret)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_MEMBER_SET', (event, processId, routingId, id, name, value) ->
  try
    obj = objectsRegistry.get id
    obj[name] = value
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_MEMBER_GET', (event, processId, routingId, id, name) ->
  try
    obj = objectsRegistry.get id
    event.result = new Meta(processId, routingId, obj[name])
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_REFERENCE', (event, processId, routingId, id) ->
  try
    obj = objectsRegistry.get id
    event.result = new Meta(processId, routingId, obj)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_BROWSER_DEREFERENCE', (processId, routingId, storeId) ->
  objectsRegistry.remove processId, routingId, storeId
