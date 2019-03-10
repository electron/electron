#!/usr/bin/env node

const childProcess = require('child_process')
const crypto = require('crypto')
const fs = require('fs')
const { hashElement } = require('folder-hash')
const path = require('path')

const utils = require('./lib/utils')

const BASE = path.resolve(__dirname, '../..')
const NPM_CMD = process.platform === 'win32' ? 'npm.cmd' : 'npm'

const specHashPath = path.resolve(__dirname, '../spec/.hash')

let only = null
const onlyArg = process.argv.find(arg => arg.startsWith('--only='))
if (onlyArg) {
  only = onlyArg.substr(7).split(',')
  console.log('Only running:', only)
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

  await runElectronTests()
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
  const runners = [
    ['Remote based specs', 'remote', runRemoteBasedElectronTests],
    ['Main process specs', 'main', runMainProcessElectronTests]
  ]

  const mochaFile = process.env.MOCHA_FILE
  for (const runner of runners) {
    if (only && !only.includes(runner[1])) {
      console.info('\nSkipping:', runner[0])
      continue
    }
    try {
      console.info('\nRunning:', runner[0])
      if (mochaFile) {
        process.env.MOCHA_FILE = mochaFile.replace('.xml', `-${runner[1]}.xml`)
      }
      await runner[2]()
    } catch (err) {
      errors.push([runner[0], err])
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
  const args = process.argv.slice(2).filter(arg => !arg.startsWith('--only='))
  if (process.platform === 'linux') {
    args.unshift(path.resolve(__dirname, 'dbus_mock.py'), exe)
    exe = 'python'
  }

  const { status } = childProcess.spawnSync(exe, ['electron/spec', ...args], {
    cwd: path.resolve(__dirname, '../..'),
    stdio: 'inherit'
  })
  if (status !== 0) {
    throw new Error(`Electron tests failed with code ${status}.`)
  }
}

async function runMainProcessElectronTests () {
  const exe = path.resolve(BASE, utils.getElectronExec())
  const args = process.argv.slice(2).filter(arg => !arg.startsWith('--only='))

  console.log(exe, args)

  const { status } = childProcess.spawnSync(exe, ['electron/spec-main', ...args], {
    cwd: path.resolve(__dirname, '../..'),
    stdio: 'inherit'
  })
  if (status !== 0) {
    throw new Error(`Electron tests failed with code ${status}.`)
  }
}

async function installSpecModules () {
  const nodeDir = path.resolve(BASE, `out/${utils.OUT_DIR}/gen/node_headers`)
  const env = Object.assign({}, process.env, {
    npm_config_nodedir: nodeDir,
    npm_config_msvs_version: '2017'
  })
  const { status } = childProcess.spawnSync(NPM_CMD, ['install'], {
    env,
    cwd: path.resolve(__dirname, '../spec'),
    stdio: 'inherit'
  })
  if (status !== 0) {
    throw new Error('Failed to npm install in the spec folder')
  }
}

function getSpecHash () {
  return Promise.all([
    (async () => {
      const hasher = crypto.createHash('SHA256')
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/package.json')))
      hasher.update(fs.readFileSync(path.resolve(__dirname, '../spec/package-lock.json')))
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
