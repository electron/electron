EventEmitter = require('events').EventEmitter
v8Util = process.atomBinding 'v8_util'

class ObjectsRegistry extends EventEmitter
  constructor: ->
    @setMaxListeners Number.MAX_VALUE
    @nextId = 0

    # Stores all objects by ref-counting.
    # (id) => {object, count}
    @storage = {}

    # Stores the IDs of objects referenced by WebContents.
    # (webContentsId) => {(id) => (count)}
    @owners = {}

  # Register a new object, the object would be kept referenced until you release
  # it explicitly.
  add: (webContentsId, obj) ->
    id = @saveToStorage obj
    # Remember the owner.
    @owners[webContentsId] ?= {}
    @owners[webContentsId][id] ?= 0
    @owners[webContentsId][id]++
    # Returns object's id
    id

  # Get an object according to its ID.
  get: (id) ->
    @storage[id]?.object

  # Dereference an object according to its ID.
  remove: (webContentsId, id) ->
    @dereference id, 1
    # Also reduce the count in owner.
    pointer = @owners[webContentsId]
    return unless pointer?
    --pointer[id]
    delete pointer[id] if pointer[id] is 0

  # Clear all references to objects refrenced by the WebContents.
  clear: (webContentsId) ->
    @emit "clear-#{webContentsId}"
    return unless @owners[webContentsId]?
    @dereference id, count for id, count of @owners[webContentsId]
    delete @owners[webContentsId]

  # Private: Saves the object into storage and assigns an ID for it.
  saveToStorage: (object) ->
    id = v8Util.getHiddenValue object, 'atomId'
    unless id
      id = ++@nextId
      @storage[id] = {count: 0, object}
      v8Util.setHiddenValue object, 'atomId', id
    ++@storage[id].count
    id

  # Private: Dereference the object from store.
  dereference: (id, count) ->
    pointer = @storage[id]
    return unless pointer?
    pointer.count -= count
    if pointer.count is 0
      v8Util.deleteHiddenValue pointer.object, 'atomId'
      delete @storage[id]

module.exports = new ObjectsRegistry
