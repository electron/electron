'use strict'

const { remote } = require('electron')

exports.getRemote = function (name) {
  if (!remote) {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
  return remote[name]
}
