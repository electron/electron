savedGlobal = global  # the "global.global" might be deleted later

module.exports =
class CallbacksRegistry
  constructor: ->
    @emptyFunc = -> throw new Error "Browser trying to call a non-exist callback
      in renderer, this usually happens when renderer code forgot to release
      a callback installed on objects in browser when renderer was going to be
      unloaded or released."
    @callbacks = {}

  add: (callback) ->
    id = Math.random().toString()
    @callbacks[id] = callback
    id

  get: (id) ->
    @callbacks[id] ? ->

  call: (id, args...) ->
    @get(id).call savedGlobal, args...

  apply: (id, args...) ->
    @get(id).apply savedGlobal, args...

  remove: (id) ->
    delete @callbacks[id]
