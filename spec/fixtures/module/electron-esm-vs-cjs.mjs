import * as esmElectron from 'electron';

import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const cjsElectron = require('electron');

process.stdout.write(
  JSON.stringify({
    esm: Object.keys(esmElectron).sort(),
    cjs: Object.keys(cjsElectron).sort()
  })
);
process.exit(0);
