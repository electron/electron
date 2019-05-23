#!/usr/bin/env node

const version = require('./package').version

const fs = require('fs')
const os = require('os')
const path = require('path')
const extract = require('extract-zip')
const { downloadArtifact } = require('@electron/get')

let installedVersion = null
try {
  installedVersion = fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '')
} catch (ignored) {
  // do nothing
}

if (process.env.ELECTRON_SKIP_BINARY_DOWNLOAD) {
  process.exit(0)
}

const platformPath = getPlatformPath()

const electronPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist', platformPath)

if (installedVersion === version && fs.existsSync(electronPath)) {
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
}).then((zipPath) => extractFile(zipPath)).catch((err) => onerror(err))

// unzips and makes path.txt point at the correct executable
function extractFile (zipPath) {
  extract(zipPath, { dir: path.join(__dirname, 'dist') }, function (err) {
    if (err) return onerror(err)
    fs.writeFile(path.join(__dirname, 'path.txt'), platformPath, function (err) {
      if (err) return onerror(err)
    })
  })
}

function onerror (err) {
  throw err
}

function getPlatformPath () {
  const platform = process.env.npm_config_platform || os.platform()

  switch (platform) {
    case 'darwin':
      return 'Electron.app/Contents/MacOS/Electron'
    case 'freebsd':
    case 'linux':
      return 'electron'
    case 'win32':
      return 'electron.exe'
    default:
      throw new Error('Electron builds are not available on platform: ' + platform)
  }
}
