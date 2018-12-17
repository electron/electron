'use strict'

const { remote } = require('electron')

exports.getRemote = function (name) {
  if (!remote) {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
  return remote[name]
}

exports.remoteRequire = function (name) {
  if (!remote) {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
  return remote.require(name)
}

exports.potentiallyRemoteRequire = function (name) {
  if (process.sandboxed) {
    return exports.remoteRequire(name)
  } else {
    return require(name)
  }
}
