const cp = require('child_process')
const fs = require('fs')
const path = require('path')

const BASE = path.resolve(__dirname, '../..')
const NODE_DIR = path.resolve(BASE, 'third_party', 'electron_node')
const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx'

const utils = require('./lib/utils')
const { YARN_VERSION } = require('./yarn')

if (!process.mainModule) {
  throw new Error('Must call the node spec runner directly')
}

async function main () {
  const DISABLED_TESTS = require('./node-disabled-tests.json')

  const testChild = cp.spawn('python', ['tools/test.py', '--verbose', '-p', 'tap', '--logfile', 'test.tap', '--mode=debug', 'default', `--skip-tests=${DISABLED_TESTS.join(',')}`, '--shell', utils.getAbsoluteElectronExec(), '-J'], {
    env: {
      ...process.env,
      ELECTRON_RUN_AS_NODE: 'true'
    },
    cwd: NODE_DIR,
    stdio: 'inherit'
  })
  testChild.on('exit', (testCode) => {
    process.exit(testCode)
  })
}

main().catch((err) => {
  console.error('An unhandled error occurred in the node spec runner', err)
  process.exit(1)
})
