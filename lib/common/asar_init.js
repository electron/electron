'use strict'

/* global require */

// Monkey-patch the fs module.
require('electron/js2c/asar').wrapFsWithAsar(require('fs'))
