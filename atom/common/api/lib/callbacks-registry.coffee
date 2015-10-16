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

    # Capture the location of the function and put it in the ID string,
    # so that release errors can be tracked down easily.
    regexp = /at (.*)/gi
    stackString = (new Error).stack

    while (match = regexp.exec(stackString)) isnt null
      [x, location] = match
      continue if location.indexOf('(native)') isnt -1
      continue if location.indexOf('atom.asar') isnt -1
      [x, filenameAndLine] = /([^/^\)]*)\)?$/gi.exec(location)
      id = "#{filenameAndLine} (#{id})"
      break

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
