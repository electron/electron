'use strict'

const { remote } = require('electron')

exports.getRemoteForUsage = function (usage) {
  if (!remote) {
    throw new Error(`${usage} requires remote, which is not enabled`)
  }
  return remote
}
