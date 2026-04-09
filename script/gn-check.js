/*
Usage:

$ node ./script/gn-check.js [--outDir=dirName]
*/

const minimist = require('minimist');

const cp = require('node:child_process');
const path = require('node:path');

const args = minimist(process.argv.slice(2), { string: ['outDir'] });

const { getDepotToolsEnv, getOutDir } = require('./lib/utils');

const SOURCE_ROOT = path.normalize(path.dirname(__dirname));

const OUT_DIR = getOutDir({ outDir: args.outDir });
if (!OUT_DIR) {
  throw new Error('No viable out dir: one of Debug, Testing, or Release must exist.');
}

const env = {
  ...getDepotToolsEnv(),
  CHROMIUM_BUILDTOOLS_PATH: path.resolve(SOURCE_ROOT, '..', 'buildtools'),
  DEPOT_TOOLS_WIN_TOOLCHAIN: '0',
};

const gnCheckDirs = [
  '//electron:electron_lib',
  '//electron:electron_app',
  '//electron/shell/common:mojo',
  '//electron/shell/common:plugin',
  '//electron:testing_build',
  '//electron:release_build'
];

for (const dir of gnCheckDirs) {
  const args = ['check', `../out/${OUT_DIR}`, dir];
  const result = cp.spawnSync('gn', args, { env, stdio: 'inherit' });
  if (result.status !== 0) process.exit(result.status);
}

process.exit(0);
