const { execFileSync } = require('node:child_process');

const [, , exe, cacheDir] = process.argv;

execFileSync(exe, ['-e', "console.log('compile cache:', require('node:module').getCompileCacheDir() !== undefined)"], {
  env: { ELECTRON_RUN_AS_NODE: 'true', NODE_COMPILE_CACHE: cacheDir },
  stdio: 'inherit'
});
process.exit(0);
