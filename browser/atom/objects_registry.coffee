module.exports =
class ObjectsRegistry
  @nextId = 0

  constructor: ->
    @objects = []

  getNextId: ->
    ++ObjectsRegistry.nextId

  add: (obj) ->
    id = @getNextId()
    @objects[id] = obj
    id

  remove: (id) ->
    obj = @objects[id]
    delete @objects[id]
    obj

  get: (id) ->
    @objects[id]
