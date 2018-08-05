/* global binding */

'use strict'

const {send, sendSync} = binding

const ipcRenderer = {
  send (...args) {
    return send('ipc-message', args)
  },

  sendSync (...args) {
    return sendSync('ipc-message-sync', args)[0]
  },

  // No-ops since events aren't received
  on () {},
  once () {}
}

let {guestInstanceId, hiddenPage, openerId, nativeWindowOpen} = binding
if (guestInstanceId != null) guestInstanceId = parseInt(guestInstanceId)
if (openerId != null) openerId = parseInt(openerId)

require('../renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage, nativeWindowOpen)
