BrowserWindow = require 'browser-window'
EventEmitter = require('events').EventEmitter
IDWeakMap = require 'id-weak-map'
v8Util = process.atomBinding 'v8_util'

# Class to reference all objects.
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

class ObjectsRegistry extends EventEmitter
  constructor: ->
    @setMaxListeners Number.MAX_VALUE

    # Objects in weak map will be not referenced (so we won't leak memory), and
    # every object created in browser will have a unique id in weak map.
    @objectsWeakMap = new IDWeakMap
    @objectsWeakMap.add = (obj) ->
      id = IDWeakMap::add.call this, obj
      v8Util.setHiddenValue obj, 'atomId', id
      id

    # Remember all windows in the weak map.
    @windowsWeakMap = new IDWeakMap
    process.on 'ATOM_BROWSER_INTERNAL_NEW', (obj) =>
      if obj.constructor is BrowserWindow
        id = @windowsWeakMap.add obj
        obj.on 'destroyed', => @windowsWeakMap.remove id

  # Register a new object, the object would be kept referenced until you release
  # it explicitly.
  add: (processId, routingId, obj) ->
    # Some native objects may already been added to objectsWeakMap, be care not
    # to add it twice.
    @objectsWeakMap.add obj unless v8Util.getHiddenValue obj, 'atomId'
    id = v8Util.getHiddenValue obj, 'atomId'

    # Store and reference the object, then return the storeId which points to
    # where the object is stored. The caller can later dereference the object
    # with the storeId.
    # We use a difference key because the same object can be referenced for
    # multiple times by the same renderer view.
    store = ObjectsStore.forRenderView processId, routingId
    storeId = store.add obj

    [id, storeId]

  # Get an object according to its id.
  get: (id) ->
    @objectsWeakMap.get id

  # Remove an object according to its storeId.
  remove: (processId, routingId, storeId) ->
    ObjectsStore.forRenderView(processId, routingId).remove storeId

  # Clear all references to objects from renderer view.
  clear: (processId, routingId) ->
    @emit "release-renderer-view-#{processId}-#{routingId}"
    ObjectsStore.releaseForRenderView processId, routingId

  # Return an array of all browser windows.
  getAllWindows: ->
    keys = @windowsWeakMap.keys()
    @windowsWeakMap.get key for key in keys

module.exports = new ObjectsRegistry
