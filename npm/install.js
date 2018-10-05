#!/usr/bin/env node

const version = require('./package').version

const fs = require('fs')
const os = require('os')
const path = require('path')
const extract = require('extract-zip')
const download = require('electron-download')

let installedVersion = null
try {
  installedVersion = fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '')
} catch (ignored) {
  // do nothing
}

const platformPath = getPlatformPath()

const electronPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist', platformPath)

if (installedVersion === version && fs.existsSync(electronPath)) {
  process.exit(0)
}

// downloads if not cached
download({
  cache: process.env.electron_config_cache,
  version: version,
  platform: process.env.npm_config_platform,
  arch: process.env.npm_config_arch,
  strictSSL: process.env.npm_config_strict_ssl === 'true',
  force: process.env.force_no_cache === 'true',
  quiet: process.env.npm_config_loglevel === 'silent' || process.env.CI
}, extractFile)

// unzips and makes path.txt point at the correct executable
function extractFile (err, zipPath) {
  if (err) return onerror(err)
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
