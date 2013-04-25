ipc = require 'ipc'
path = require 'path'
ObjectsRegistry = require './objects_registry.js'

objectsRegistry = new ObjectsRegistry

class PlainObject
  constructor: (value) ->
    @type = typeof value
    @type = 'value' if value is null
    @type = 'array' if Array.isArray value

    if @type is 'array'
      @members = []
      @members.push new PlainObject(el) for el in value
    else if @type is 'object' or @type is 'function'
      @name = value.constructor.name
      @id = objectsRegistry.add value

      @members = []
      @members.push { name: prop, type: typeof field } for prop, field of value
    else
      @type = 'value'
      @value = value

ipc.on 'ATOM_INTERNAL_REQUIRE', (event, process_id, routing_id, module) ->
  event.result = new PlainObject(require(module))

ipc.on 'ATOM_INTERNAL_CONSTRUCTOR', (event, process_id, routing_id, id, args) ->
  try
    constructor = objectsRegistry.get id
    # Call new with array of arguments.
    # http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    obj = new (Function::bind.apply(constructor, [null].concat(args)))
    event.result = new PlainObject(obj)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_INTERNAL_FUNCTION_CALL', (event, process_id, routing_id, id, args) ->
  try
    ret = objectsRegistry.get(id).apply global, args
    event.result = new PlainObject(ret)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_INTERNAL_MEMBER_CALL', (event, process_id, routing_id, id, method, args) ->
  try
    obj = objectsRegistry.get id
    ret = obj[method].apply(obj, args)
    event.result = new PlainObject(ret)
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_INTERNAL_MEMBER_SET', (event, process_id, routing_id, id, name, value) ->
  try
    objectsRegistry.get(id)[name] = value
  catch e
    event.result = type: 'error', value: e.message

ipc.on 'ATOM_INTERNAL_MEMBER_GET', (event, process_id, routing_id, id, name) ->
  try
    event.result = new PlainObject(objectsRegistry.get(id)[name])
  catch e
    event.result = type: 'error', value: e.message
