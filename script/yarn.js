const path = require('node:path');

exports.YARN_SCRIPT_PATH = path.resolve(__dirname, '..', '.yarn/releases/yarn-4.12.0.cjs');

if (require.main === module) {
  require(exports.YARN_SCRIPT_PATH);
}
