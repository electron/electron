import { parseDocs } from '@electron/docs-parser';
import { promises } from 'node:fs';
import { resolve } from 'node:path';

import { getElectronVersion } from './lib/get-version.js';

parseDocs({
  baseDirectory: resolve(import.meta.dirname, '..'),
  packageMode: 'single',
  useReadme: false,
  moduleVersion: getElectronVersion()
}).then((api) => {
  return promises.writeFile(resolve(import.meta.dirname, '..', 'electron-api.json'), JSON.stringify(api, null, 2));
}).catch((err) => {
  console.error(err);
  process.exit(1);
});
