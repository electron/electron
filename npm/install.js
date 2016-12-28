#!/usr/bin/env node

// maintainer note - x.y.z-ab version in package.json -> x.y.z
var version = require('./package').version.replace(/-.*/, '')

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

// downloads if not cached
download({
  version: version,
  platform: process.env.npm_config_platform,
  arch: process.env.npm_config_arch,
  strictSSL: process.env.npm_config_strict_ssl === 'true',
  quiet: ['info', 'verbose', 'silly', 'http'].indexOf(process.env.npm_config_loglevel) === -1
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
