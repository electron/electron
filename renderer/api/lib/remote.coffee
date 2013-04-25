ipc = require 'ipc'

generateFromPainObject = (plain) ->
  if plain.type is 'value'
    return plain.value
  else if plain.type is 'error'
    throw new Error('Remote Error: ' + plain.value)

  # A shadow class to represent the remote object.
  class RemoteObject
    constructor: () ->
      if @constructor == RemoteObject
        # Constructor call.
        obj = ipc.sendChannelSync 'ATOM_INTERNAL_CONSTRUCTOR', plain.id, Array::slice.call(arguments)

        # Returning object in constructor will replace constructed object
        # with the returned object.
        # http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
        return generateFromPainObject obj
      else
        # Function call.
        ret = ipc.sendChannelSync 'ATOM_INTERNAL_FUNCTION_CALL', plain.id, Array::slice.call(arguments)
        generateFromPainObject ret

  # Polulate shadow members.
  for member in plain.members
    do (member) ->
      if member.type is 'function'
        RemoteObject[member.name] = ->
          # Call member function.
          ret = ipc.sendChannelSync 'ATOM_INTERNAL_MEMBER_CALL', plain.id, member.name, Array::slice.call(arguments)
          generateFromPainObject ret
      else
        RemoteObject.__defineSetter__ member.name, (value) ->
          # Set member data.
          ipc.sendChannelSync 'ATOM_INTERNAL_MEMBER_SET', plain.id, member.name, value
          undefined

        RemoteObject.__defineGetter__ member.name, ->
          # Get member data.
          ret = ipc.sendChannelSync 'ATOM_INTERNAL_MEMBER_GET', plain.id, member.name
          generateFromPainObject ret

  RemoteObject

exports.require = (module) ->
  plain = ipc.sendChannelSync 'ATOM_INTERNAL_REQUIRE', module
  generateFromPainObject plain
