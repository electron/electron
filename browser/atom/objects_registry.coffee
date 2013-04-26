IDWeakMap = require 'id_weak_map'

globalStore = {}
globalMap = new IDWeakMap

getStoreForRenderView = (process_id, routing_id) ->
  key = "#{process_id}_#{routing_id}"
  globalStore[key] = {} unless globalStore[key]?
  globalStore[key]

process.on 'ATOM_BROWSER_INTERNAL_NEW', (obj) ->
  # For objects created in browser scripts, keep a weak reference here.
  id = globalMap.add obj
  obj.id = id

exports.add = (process_id, routing_id, obj) ->
  # Some native types may already been added to globalMap, in that case we
  # don't add it twice.
  id = obj.id ? globalMap.add obj

  store = getStoreForRenderView process_id, routing_id

  # It's possible that a render view may want to get the same remote object
  # twice, since we only allow one reference of one object per render view,
  # we throw when the object is already referenced.
  throw new Error("Object #{id} is already referenced") if store[id]?

  store[id] = obj
  id

exports.get = (id) ->
  globalMap.get id

exports.remove = (process_id, routing_id, id) ->
  store = getStoreForRenderView process_id, routing_id
  delete store[id]
