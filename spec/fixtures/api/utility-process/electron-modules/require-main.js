const { net } = require('electron/main');

process.exit(net !== undefined ? 0 : 1);
