const { net } = require('electron/utility');

process.exit(net !== undefined ? 0 : 1);
