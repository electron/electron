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
    delete @objects[id]

  get: (id) ->
    throw new Error("Invalid key #{id} for ObjectsStore") unless @has id
    @objects[id]

  @forRenderView: (process_id, routing_id) ->
    key = "#{process_id}_#{routing_id}"
    @stores[key] = new ObjectsStore unless @stores[key]?
    @stores[key]

objectsWeakMap = new IDWeakMap
objectsWeakMap.add = (obj) ->
  id = IDWeakMap::add.call this, obj
  Object.defineProperty obj, 'id',
    enumerable: true, writable: false, value: id
  id

process.on 'ATOM_BROWSER_INTERNAL_NEW', (obj) ->
  # It's possible that user created a object in browser side and then want to
  # get it in renderer via remote.getObject. So we must add every native object
  # created in browser to the weak map.
  objectsWeakMap.add obj

exports.add = (process_id, routing_id, obj) ->
  # Some native types may already been added to objectsWeakMap, in that case we
  # don't add it twice.
  objectsWeakMap.add obj unless obj.id?

  store = ObjectsStore.forRenderView process_id, routing_id
  store.add obj

exports.get = (id) ->
  objectsWeakMap.get id

exports.remove = (process_id, routing_id, storeId) ->
  ObjectsStore.forRenderView(process_id, routing_id).remove storeId
