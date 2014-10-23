ipc = require 'ipc'

requestId = 0

module.exports =
  createGuest: (type, params, callback) ->
    requestId++
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', type, params, requestId
    ipc.on "ATOM_SHELL_RESPONSE_#{requestId}", callback

  destroyGuest: (guestInstanceId) ->
    ipc.send 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId
