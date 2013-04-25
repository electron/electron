ipc = require 'ipc'
v8_util = process.atom_binding 'v8_util'

generateFromPainObject = (plain) ->
  switch plain.type
    when 'error' then throw new Error('Remote Error: ' + plain.value)
    when 'value' then plain.value
    when 'array' then (generateFromPainObject(el) for el in plain.members)
    else
      if plain.type is 'function'
        # A shadow class to represent the remote function object.
        ret =
        class RemoteFunction
          constructor: ->
            if @constructor == RemoteFunction
              # Constructor call.
              obj = ipc.sendChannelSync 'ATOM_INTERNAL_CONSTRUCTOR', plain.id, Array::slice.call(arguments)

              # Returning object in constructor will replace constructed object
              # with the returned object.
              # http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
              return generateFromPainObject obj
            else
              # Function call.
              ret = ipc.sendChannelSync 'ATOM_INTERNAL_FUNCTION_CALL', plain.id, Array::slice.call(arguments)
              return generateFromPainObject ret
      else
        ret = v8_util.createObjectWithName plain.name

      # Polulate delegate members.
      for member in plain.members
        do (member) ->
          if member.type is 'function'
            ret[member.name] = ->
              # Call member function.
              ret = ipc.sendChannelSync 'ATOM_INTERNAL_MEMBER_CALL', plain.id, member.name, Array::slice.call(arguments)
              generateFromPainObject ret
          else
            ret.__defineSetter__ member.name, (value) ->
              # Set member data.
              ipc.sendChannelSync 'ATOM_INTERNAL_MEMBER_SET', plain.id, member.name, value

            ret.__defineGetter__ member.name, ->
              # Get member data.
              ret = ipc.sendChannelSync 'ATOM_INTERNAL_MEMBER_GET', plain.id, member.name
              generateFromPainObject ret

      ret

exports.require = (module) ->
  plain = ipc.sendChannelSync 'ATOM_INTERNAL_REQUIRE', module
  generateFromPainObject plain
