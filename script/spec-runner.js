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
  let exe = path.resolve(BASE, utils.getElectronExec())
  const args = process.argv.slice(2)
  if (process.platform === 'linux') {
    args.unshift(path.resolve(__dirname, 'dbus_mock.py'), exe)
    exe = 'python'
  }

  const { status } = childProcess.spawnSync(exe, args, {
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
