const { net } = require('electron/common');

process.exit(net !== undefined ? 0 : 1);
