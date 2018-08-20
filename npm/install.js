#!/usr/bin/env node

var version = require('./package').version

var fs = require('fs')
var os = require('os')
var path = require('path')
var extract = require('extract-zip')
var download = require('electron-download')

var installedVersion = null
try {
  installedVersion = fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '')
} catch (ignored) {
  // do nothing
}

var platformPath = getPlatformPath()

if (installedVersion === version && fs.existsSync(path.join(__dirname, platformPath))) {
  process.exit(0)
}

var mirror

if (version.indexOf('nightly') !== -1) {
  mirror = 'https://github.com/electron/nightlies/releases/download/v'
}

// downloads if not cached
download({
  cache: process.env.electron_config_cache,
  version: version,
  platform: process.env.npm_config_platform,
  arch: process.env.npm_config_arch,
  strictSSL: process.env.npm_config_strict_ssl === 'true',
  force: process.env.force_no_cache === 'true',
  quiet: process.env.npm_config_loglevel === 'silent' || process.env.CI,
  mirror
}, extractFile)

// unzips and makes path.txt point at the correct executable
function extractFile (err, zipPath) {
  if (err) return onerror(err)
  extract(zipPath, {dir: path.join(__dirname, 'dist')}, function (err) {
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
  var platform = process.env.npm_config_platform || os.platform()

  switch (platform) {
    case 'darwin':
      return 'dist/Electron.app/Contents/MacOS/Electron'
    case 'freebsd':
    case 'linux':
      return 'dist/electron'
    case 'win32':
      return 'dist/electron.exe'
    default:
      throw new Error('Electron builds are not available on platform: ' + platform)
  }
}
