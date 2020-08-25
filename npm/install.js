#!/usr/bin/env node

const {version} = require('./package')

const fs = require('fs')
const os = require('os')
const path = require('path')
const extract = require('extract-zip')
const { downloadArtifact } = require('@electron/get')

if (process.env.ELECTRON_SKIP_BINARY_DOWNLOAD) {
  process.exit(0)
}

const platformPath = getPlatformPath()

if (isInstalled()) {
  process.exit(0)
}

// downloads if not cached
downloadArtifact({
  version,
  artifactName: 'electron',
  force: process.env.force_no_cache === 'true',
  cacheRoot: process.env.electron_config_cache,
  platform: process.env.npm_config_platform || process.platform,
  arch: process.env.npm_config_arch || process.arch
}).then(extractFile).catch(err => {
  console.error(err.stack)
  process.exit(1)
})

function isInstalled () {
  try {
    if (fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '') !== version) {
      return false
    }

    if (fs.readFileSync(path.join(__dirname, 'path.txt'), 'utf-8') !== platformPath) {
      return false
    }
  } catch (ignored) {
    return false
  }
  
  const electronPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist', platformPath)
  
  return fs.existsSync(electronPath)
}

// unzips and makes path.txt point at the correct executable
function extractFile (zipPath) {
  return new Promise((resolve, reject) => {
    extract(zipPath, { dir: path.join(__dirname, 'dist') }, err => {
      if (err) return reject(err)

      fs.writeFile(path.join(__dirname, 'path.txt'), platformPath, err => {
        if (err) return reject(err)

        resolve()
      })
    })
  })
}

function getPlatformPath () {
  const platform = process.env.npm_config_platform || os.platform()

  switch (platform) {
    case 'mas':
    case 'darwin':
      return 'Electron.app/Contents/MacOS/Electron'
    case 'freebsd':
    case 'openbsd':
    case 'linux':
      return 'electron'
    case 'win32':
      return 'electron.exe'
    default:
      throw new Error('Electron builds are not available on platform: ' + platform)
  }
}
