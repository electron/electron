;(function () {
  return function (process, require, asarSource) {
    const source = process.binding('natives')

    // Expose fs module without asar support.
    source['original-fs'] = source.fs

    // Make asar.js accessible via "require".
    source.ELECTRON_ASAR = asarSource

    // Monkey-patch the fs module.
    require('ELECTRON_ASAR').wrapFsWithAsar(require('fs'))
  }
})()
