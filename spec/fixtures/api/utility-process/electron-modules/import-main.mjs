import { net } from 'electron/main';

process.exit(net !== undefined ? 0 : 1);
