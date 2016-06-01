;(function () {
  return function (process, require, asarSource) {
    // Make asar.js accessible via "require".
    process.binding('natives').ELECTRON_ASAR = asarSource

    // Monkey-patch the fs module.
    require('ELECTRON_ASAR').wrapFsWithAsar(require('fs'))

    // Make graceful-fs work with asar.
    var source = process.binding('natives')
    source['original-fs'] = source.fs
    source['fs'] = `
var nativeModule = new process.NativeModule('original-fs')
nativeModule.cache()
nativeModule.compile()
var asar = require('ELECTRON_ASAR')
asar.wrapFsWithAsar(nativeModule.exports)
module.exports = nativeModule.exports`
  }
})()
