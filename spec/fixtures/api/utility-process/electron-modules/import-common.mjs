import { net } from 'electron/common';

process.exit(net !== undefined ? 0 : 1);
