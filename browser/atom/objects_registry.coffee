IDWeakMap = require 'id_weak_map'

globalStore = {}
globalMap = new IDWeakMap

getStoreForRenderView = (process_id, routing_id) ->
  key = "#{process_id}_#{routing_id}"
  globalStore[key] = {} unless globalStore[key]?
  globalStore[key]

exports.add = (process_id, routing_id, obj) ->
  id = globalMap.add obj
  store = getStoreForRenderView process_id, routing_id
  store[id] = obj
  id

exports.get = (process_id, routing_id, id) ->
  globalMap.get id

exports.remove = (process_id, routing_id, id) ->
  store = getStoreForRenderView process_id, routing_id
  delete store[id]
