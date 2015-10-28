v8Util = process.atomBinding 'v8_util'

module.exports =
class CallbacksRegistry
  constructor: ->
    @nextId = 0
    @callbacks = {}

  add: (callback) ->
    id = ++@nextId

    # Capture the location of the function and put it in the ID string,
    # so that release errors can be tracked down easily.
    regexp = /at (.*)/gi
    stackString = (new Error).stack

    while (match = regexp.exec(stackString)) isnt null
      [x, location] = match
      continue if location.indexOf('(native)') isnt -1
      continue if location.indexOf('atom.asar') isnt -1
      [x, filenameAndLine] = /([^/^\)]*)\)?$/gi.exec(location)
      break

    if v8Util.getHiddenValue(callback, 'metaId')?
      return v8Util.getHiddenValue(callback, 'metaId')

    @callbacks[id] = callback
    v8Util.setHiddenValue callback, 'metaId', id
    v8Util.setHiddenValue callback, 'location', filenameAndLine
    id

  get: (id) ->
    @callbacks[id] ? ->

  call: (id, args...) ->
    @get(id).call global, args...

  apply: (id, args...) ->
    @get(id).apply global, args...

  remove: (id) ->
    delete @callbacks[id]
