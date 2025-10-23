import { net } from 'electron/utility';

process.exit(net !== undefined ? 0 : 1);
