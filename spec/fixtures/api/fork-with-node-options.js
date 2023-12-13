const { execFileSync } = require('node:child_process');
const path = require('node:path');

const fixtures = path.resolve(__dirname, '..');

const env = {
  ELECTRON_RUN_AS_NODE: 'true',
  // Process will exit with 1 if NODE_OPTIONS is accepted.
  NODE_OPTIONS: `--require "${path.join(fixtures, 'module', 'fail.js')}"`
};
// Provide a lower cased NODE_OPTIONS in case some code ignores case sensitivity
// when reading NODE_OPTIONS.
env.node_options = env.NODE_OPTIONS;
try {
  execFileSync(process.argv[2],
    ['--require', path.join(fixtures, 'module', 'noop.js')],
    { env, stdio: 'inherit' });
  process.exit(0);
} catch (error) {
  console.log('NODE_OPTIONS passed to child');
  process.exit(1);
}
