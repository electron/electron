import { net } from 'electron/renderer';

process.exit(net !== undefined ? 0 : 1);
