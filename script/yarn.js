const cp = require('child_process');
const fs = require('fs');
const path = require('path');

const YARN_VERSION = /'yarn_version': '(.+?)'/.exec(fs.readFileSync(path.resolve(__dirname, '../DEPS'), 'utf8'))[1];

exports.YARN_VERSION = YARN_VERSION;

// If we are running "node script/yarn" run as the yarn CLI
if (process.mainModule === module) {
  const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx';

  const child = cp.spawn(NPX_CMD, [`yarn@${YARN_VERSION}`, ...process.argv.slice(2)], {
    stdio: 'inherit',
    env: {
      ...process.env,
      npm_config_yes: 'true'
    }
  });

  child.on('exit', code => process.exit(code));
}
