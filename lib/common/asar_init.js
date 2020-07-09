'use strict';

// Monkey-patch the fs module.
require('electron/js2c/asar').wrapFsWithAsar(require('fs'));
