import { net } from 'electron/lol';

process.exit(net !== undefined ? 0 : 1);
