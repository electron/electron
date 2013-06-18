BrowserWindow = require 'browser-window'
IDWeakMap = require 'id-weak-map'

class ObjectsStore
  @stores = {}

  constructor: ->
    @nextId = 0
    @objects = []

  getNextId: ->
    ++@nextId

  add: (obj) ->
    id = @getNextId()
    @objects[id] = obj
    id

  has: (id) ->
    @objects[id]?

  remove: (id) ->
    throw new Error("Invalid key #{id} for ObjectsStore") unless @has id
    delete @objects[id]

  get: (id) ->
    throw new Error("Invalid key #{id} for ObjectsStore") unless @has id
    @objects[id]

  @forRenderView: (processId, routingId) ->
    key = "#{processId}_#{routingId}"
    @stores[key] = new ObjectsStore unless @stores[key]?
    @stores[key]

  @releaseForRenderView: (processId, routingId) ->
    key = "#{processId}_#{routingId}"
    delete @stores[key]

# Objects in weak map will be not referenced (so we won't leak memory), and
# every object created in browser will have a unique id in weak map.
objectsWeakMap = new IDWeakMap
objectsWeakMap.add = (obj) ->
  id = IDWeakMap::add.call this, obj
  Object.defineProperty obj, 'id',
    enumerable: true, writable: false, value: id
  id

windowsWeakMap = new IDWeakMap

process.on 'ATOM_BROWSER_INTERNAL_NEW_BROWSER_WINDOW', (obj) ->
  # Remember all windows.
  id = windowsWeakMap.add obj
  obj.on 'destroyed', ->
    windowsWeakMap.remove id

exports.add = (processId, routingId, obj) ->
  # Some native objects may already been added to objectsWeakMap, be care not
  # to add it twice.
  objectsWeakMap.add obj unless obj.id? and objectsWeakMap.has obj.id

  # Store and reference the object, then return the storeId which points to
  # where the object is stored. The caller can later dereference the object
  # with the storeId.
  store = ObjectsStore.forRenderView processId, routingId
  store.add obj

exports.get = (id) ->
  objectsWeakMap.get id

exports.getAllWindows = () ->
  keys = windowsWeakMap.keys()
  windowsWeakMap.get key for key in keys

exports.remove = (processId, routingId, storeId) ->
  ObjectsStore.forRenderView(processId, routingId).remove storeId

exports.clear = (processId, routingId) ->
  ObjectsStore.releaseForRenderView processId, routingId
