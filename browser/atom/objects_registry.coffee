IDWeakMap = require 'id_weak_map'

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

  @forRenderView: (process_id, routing_id) ->
    key = "#{process_id}_#{routing_id}"
    @stores[key] = new ObjectsStore unless @stores[key]?
    @stores[key]

# Objects in weak map will be not referenced (so we won't leak memory), and
# every object created in browser will have a unique id in weak map.
objectsWeakMap = new IDWeakMap
objectsWeakMap.add = (obj) ->
  id = IDWeakMap::add.call this, obj
  Object.defineProperty obj, 'id',
    enumerable: true, writable: false, value: id
  id

windowsWeakMap = new IDWeakMap

process.on 'ATOM_BROWSER_INTERNAL_NEW', (obj) ->
  # It's possible that user created a object in browser side and then want to
  # get it in renderer via remote.getObject. So we must add every native object
  # created in browser to the weak map even it may not be referenced by the
  # renderer.
  objectsWeakMap.add obj

  # Also remember all windows.
  windowsWeakMap.add obj if obj.constructor.name is 'Window'

exports.add = (process_id, routing_id, obj) ->
  # Some native objects may already been added to objectsWeakMap, be care not
  # to add it twice.
  objectsWeakMap.add obj unless obj.id?

  # Store and reference the object, then return the storeId which points to
  # where the object is stored. The caller can later dereference the object
  # with the storeId.
  store = ObjectsStore.forRenderView process_id, routing_id
  store.add obj

exports.get = (id) ->
  objectsWeakMap.get id

exports.getAllWindows = () ->
  keys = windowsWeakMap.keys()
  windowsWeakMap.get key for key in keys

exports.remove = (process_id, routing_id, storeId) ->
  ObjectsStore.forRenderView(process_id, routing_id).remove storeId
