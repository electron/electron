;(function () {
  return function (process, require, asarSource) {
    // Make asar.coffee accessible via "require".
    process.binding('natives').ATOM_SHELL_ASAR = asarSource

    // Monkey-patch the fs module.
    require('ATOM_SHELL_ASAR').wrapFsWithAsar(require('fs'))

    // Make graceful-fs work with asar.
    var source = process.binding('natives')
    source['original-fs'] = source.fs
    return source['fs'] = `
var nativeModule = new process.NativeModule('original-fs')
nativeModule.cache()
nativeModule.compile()
var asar = require('ATOM_SHELL_ASAR')
asar.wrapFsWithAsar(nativeModule.exports)
module.exports = nativeModule.exports`
  }
})()
