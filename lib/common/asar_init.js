'use strict'

/* global source, require */

// Expose fs module without asar support.
// NB: Node's 'fs' and 'internal/fs/streams' have a lazy-loaded circular
// dependency. So to expose the unmodified Node 'fs' functionality here,
// we have to copy both 'fs' *and* 'internal/fs/streams' and modify the
// copies to depend on each other instead of on our asarified 'fs' code.
source['original-fs'].replace("require('internal/fs/streams')", "require('original-fs/streams')")
source['original-fs/streams'].replace("require('fs')", "require('original-fs')")

// Monkey-patch the fs module.
require('electron/js2c/asar').wrapFsWithAsar(require('fs'))
