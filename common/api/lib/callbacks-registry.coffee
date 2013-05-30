module.exports =
class CallbacksRegistry
  constructor: ->
    @nextId = 0
    @callbacks = {}

  add: (callback) ->
    @callbacks[++@nextId] = callback
    @nextId

  get: (id) ->
    @callbacks[id]

  call: (id, args...) ->
    @get(id).call global, args...

  apply: (id, args...) ->
    @get(id).apply global, args...

  remove: (id) ->
    delete @callbacks[id]
