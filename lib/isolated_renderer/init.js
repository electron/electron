/* global binding */

'use strict'

const {guestInstanceId, hiddenPage, openerId, send, sendSync} = binding
const {parse} = JSON

const ipcRenderer = {
  send (...args) {
    return send('ipc-message', args)
  },

  sendSync (...args) {
    return parse(sendSync('ipc-message-sync', args))
  },

  // No-ops since events aren't received
  on () {},
  once () {}
}

require('../renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage)
