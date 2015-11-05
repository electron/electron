ipc = require('ipc')
v8Util = process.atomBinding 'v8_util'

ipc.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_FIND_WEB_VIEW_REQUEST', (id, requestId) ->
  viewInstanceId = 0
  internalInstanceId = 0
  webviewNode = document.querySelector("webview[id=#{id}]");
  if webviewNode
    internal = v8Util.getHiddenValue webviewNode, 'internal'
    if internal
      viewInstanceId = internal.viewInstanceId
      internalInstanceId = internal.internalInstanceId

  ipc.send "ATOM_SHELL_GUEST_VIEW_MANAGER_FIND_WEB_VIEW_RESPONSE_#{requestId}", viewInstanceId, internalInstanceId