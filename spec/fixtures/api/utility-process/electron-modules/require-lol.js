const { net } = require('electron/lol');

process.exit(net !== undefined ? 0 : 1);
