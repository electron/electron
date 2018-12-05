'use strict'

const { remote } = require('electron')

exports.getRemote = function (name) {
  if (!remote) {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
  return remote[name]
}

exports.getRemoteFor = function (usage) {
  if (!remote) {
    throw new Error(`${usage} requires remote, which is not enabled`)
  }
  return remote
}

exports.nativeRequire = function (name) {
  if (process.sandboxed) {
    return exports.getRemoteFor(name).require(name)
  } else {
    return require(name)
  }
}
