#!/usr/bin/env node

const childProcess = require('child_process')
const crypto = require('crypto')
const fs = require('fs')
const { hashElement } = require('folder-hash')
const path = require('path')
const unknownFlags = []

const args = require('minimist')(process.argv, {
  string: ['runners'],
  unknown: arg => unknownFlags.push(arg)
})

const unknownArgs = []
for (const flag of unknownFlags) {
  unknownArgs.push(flag)
  const onlyFlag = flag.replace(/^-+/, '')
  if (args[onlyFlag]) {
    unknownArgs.push(args[onlyFlag])
  }
}

const utils = require('./lib/utils')
const { YARN_VERSION } = require('./yarn')

const BASE = path.resolve(__dirname, '../..')
const NPM_CMD = process.platform === 'win32' ? 'npm.cmd' : 'npm'
const NPX_CMD = process.platform === 'win32' ? 'npx.cmd' : 'npx'

const specHashPath = path.resolve(__dirname, '../spec/.hash')

let runnersToRun = null
if (args.runners) {
  runnersToRun = args.runners.split(',')
  console.log('Only running:', runnersToRun)
} else {
  console.log('Will trigger all spec runners')
}

async function main () {
  const [lastSpecHash, lastSpecInstallHash] = loadLastSpecHash()
  const [currentSpecHash, currentSpecInstallHash] = await getSpecHash()
  const somethingChanged = (currentSpecHash !== lastSpecHash) ||
      (lastSpecInstallHash !== currentSpecInstallHash)

  if (somethingChanged) {
    await installSpecModules()
    await getSpecHash().then(saveSpecHash)
  }

  if (!fs.existsSync(path.resolve(__dirname, '../electron.d.ts'))) {
    console.log('Generating electron.d.ts as it is missing')
    generateTypeDefinitions()
  }

  await runElectronTests()
}

function generateTypeDefinitions () {
  const { status } = childProcess.spawnSync('npm', ['run', 'create-typescript-definitions'], {
    cwd: path.resolve(__dirname, '..'),
    stdio: 'inherit'
  })
  if (status !== 0) {
    throw new Error(`Electron typescript definition generation failed with exit code: ${status}.`)
  }
}

function loadLastSpecHash () {
  return fs.existsSync(specHashPath)
    ? fs.readFileSync(specHashPath, 'utf8').split('\n')
    : [null, null]
}

function saveSpecHash ([newSpecHash, newSpecInstallHash]) {
  fs.writeFileSync(specHashPath, `${newSpecHash}\n${newSpecInstallHash}`)
}

async function runElectronTests () {
  const errors = []
  const runners = new Map([
    ['main', { description: 'Main process specs', run: runMainProcessElectronTests }],
    ['remote', { description: 'Remote based specs', run: runRemoteBasedElectronTests }]
  ])

  const testResultsDir = process.env.ELECTRON_TEST_RESULTS_DIR
  for (const [runnerId, { description, run }] of runners) {
    if (runnersToRun && !runnersToRun.includes(runnerId)) {
      console.info('\nSkipping:', description)
      continue
    }
    try {
      console.info('\nRunning:', description)
      if (testResultsDir) {
        process.env.MOCHA_FILE = path.join(testResultsDir, `test-results-${runnerId}.xml`)
      }
      await run()
    } catch (err) {
      errors.push([runnerId, err])
    }
  }

  if (errors.length !== 0) {
    for (const err of errors) {
      console.error('\n\nRunner Failed:', err[0])
      console.error(err[1])
    }
    throw new Error('Electron test runners have failed')
  }
}

async function runRemoteBasedElectronTests () {
  let exe = path.resolve(BASE, utils.getElectronExec())
  const runnerArgs = ['electron/spec', ...unknownArgs.slice(2)]
  if (process.platform === 'linux') {
    runnerArgs.unshift(path.resolve(__dirname, 'dbus_mock.py'), exe)
    exe = 'python'
  }

  const { status } = childProcess.spawnSync(exe, runnerArgs, {
    cwd: path.resolve(__dirname, '../..'),
    stdio: 'inherit'
  })
  if (status !== 0) {
    const textStatus = process.platform === 'win32' ? `0x${status.toString(16)}` : status.toString()
    throw new Error(`Electron tests failed with code ${textStatus}.`)
  }
}

async function runMainProcessElectronTests () {
  const exe = path.resolve(BASE, utils.getElectronExec())

  const { status } = childProcess.spawnSync(exe, ['electron/spec-main', ...unknownArgs.slice(2)], {
    cwd: path.resolve(__dirname, '../..'),
    stdio: 'inherit'
  })
  if (status !== 0) {
    const textStatus = process.platform === 'win32' ? `0x${status.toString(16)}` : status.toString()
    throw new Error(`Electron tests failed with code ${textStatus}.`)
  }
}

async function installSpecModules () {
  const nodeDir = path.resolve(BASE, `out/${utils.getOutDir(true)}/gen/node_headers`)
  const env = Object.assign({}, process.env, {
    npm_config_nodedir: nodeDir,
    npm_config_msvs_version: '2017'
  })
  const { status } = childProcess.spawnSync(NPX_CMD, [`yarn@${YARN_VERSION}`, 'install', '--frozen-lockfile'], {
    env,
    cwd: path.resolve(__dirname, '../spec'),
    stdio: 'inherit'
  })
  if (status !== 0 && !process.env.IGNORE_YARN_INSTALL_ERROR) {
    throw new Error('Failed to yarn install in the spec folder')
  }
}

function getSpecHash () {
  return Promise.all([
    (async () => {
      const hasher = crypto.createHash('SHA256')
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/package.json')))
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/yarn.lock')))
      return hasher.digest('hex')
    })(),
    (async () => {
      const specNodeModulesPath = path.resolve(__dirname, '../spec/node_modules')
      if (!fs.existsSync(specNodeModulesPath)) {
        return null
      }
      const { hash } = await hashElement(specNodeModulesPath, {
        folders: {
          exclude: ['.bin']
        }
      })
      return hash
    })()
  ])
}

main().catch((error) => {
  console.error('An error occurred inside the spec runner:', error)
  process.exit(1)
})
