'use strict'

;(function () { // eslint-disable-line
  return function (process, require, asarSource) {
    const source = process.binding('natives')

    // Expose fs module without asar support.

    // NB: Node's 'fs' and 'internal/fs/streams' have a lazy-loaded circular
    // dependency. So to expose the unmodified Node 'fs' functionality here,
    // we have to copy both 'fs' *and* 'internal/fs/streams' and modify the
    // copies to depend on each other instead of on our asarified 'fs' code.
    source['original-fs'] = source.fs.replace("require('internal/fs/streams')", "require('original-fs/streams')")
    source['original-fs/streams'] = source['internal/fs/streams'].replace("require('fs')", "require('original-fs')")

    // Make asar.js accessible via "require".
    source.ELECTRON_ASAR = asarSource

    // Monkey-patch the fs module.
    require('ELECTRON_ASAR').wrapFsWithAsar(require('fs'))
  }
})()
