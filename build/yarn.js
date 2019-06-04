const cp = require('child_process')

const { YARN_VERSION } = require('../script/lib/utils')

const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx'

const child = cp.spawn(NPX_CMD, [`yarn@${YARN_VERSION}`, ...process.argv.slice(2)], {
  stdio: 'inherit'
})

child.on('exit', code => process.exit(code))
