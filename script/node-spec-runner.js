const cp = require('child_process')
const fs = require('fs')
const path = require('path')

const BASE = path.resolve(__dirname, '../..')
const NODE_DIR = path.resolve(BASE, 'third_party', 'electron_node')
const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx'
const JUNIT_DIR = process.argv[2] ? path.resolve(process.argv[2]) : null
const TAP_FILE_NAME = 'test.tap'

const utils = require('./lib/utils')
const { YARN_VERSION } = require('./yarn')

if (!process.mainModule) {
  throw new Error('Must call the node spec runner directly')
}

async function main () {
  const DISABLED_TESTS = require('./node-disabled-tests.json')

  const testChild = cp.spawn('python', ['tools/test.py', '--verbose', '-p', 'tap', '--logfile', TAP_FILE_NAME, '--mode=debug', 'default', `--skip-tests=${DISABLED_TESTS.join(',')}`, '--shell', utils.getAbsoluteElectronExec(), '-J'], {
    env: {
      ...process.env,
      ELECTRON_RUN_AS_NODE: 'true',
      ELECTRON_EAGER_ASAR_HOOK_FOR_TESTING: 'true'
    },
    cwd: NODE_DIR,
    stdio: 'inherit'
  })
  testChild.on('exit', (testCode) => {
    if (JUNIT_DIR) {
      fs.mkdirSync(JUNIT_DIR)
      const converterStream = require('tap-xunit')()
      fs.createReadStream(
        path.resolve(NODE_DIR, TAP_FILE_NAME)
      ).pipe(converterStream).pipe(
        fs.createWriteStream(path.resolve(JUNIT_DIR, 'nodejs.xml'))
      ).on('close', () => {
        process.exit(testCode)
      })
    }
  })
}

main().catch((err) => {
  console.error('An unhandled error occurred in the node spec runner', err)
  process.exit(1)
})
