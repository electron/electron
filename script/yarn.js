const path = require('node:path');

exports.YARN_SCRIPT_PATH = path.resolve(__dirname, '..', '.yarn/releases/yarn-4.10.3.cjs');

if (require.main === module) {
  require(exports.YARN_SCRIPT_PATH);
}
