const { net } = require('electron/renderer');

process.exit(net !== undefined ? 0 : 1);
