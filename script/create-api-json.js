const { parseDocs } = require('@electron/docs-parser');
const fs = require('node:fs');
const path = require('node:path');

const { getElectronVersion } = require('./lib/get-version');

parseDocs({
  baseDirectory: path.resolve(__dirname, '..'),
  packageMode: 'single',
  useReadme: false,
  moduleVersion: getElectronVersion()
}).then((api) => {
  return fs.promises.writeFile(path.resolve(__dirname, '..', 'electron-api.json'), JSON.stringify(api, null, 2));
}).catch((err) => {
  console.error(err);
  process.exit(1);
});
