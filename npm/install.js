#!/usr/bin/env node

var os = require('os')
var path = require('path')
var pathExists = require('path-exists')
var mkdir = require('mkdirp')
var nugget = require('nugget')
var extract = require('extract-zip')
var fs = require('fs')
var getHomePath = require('home-path')()
var platform = os.platform()
var arch = os.arch()
var version = '0.25.3'
var filename = 'electron-v' + version + '-' + platform + '-' + arch + '.zip'
var url = 'https://github.com/atom/electron/releases/download/v' + version + '/electron-v' + version + '-' + platform + '-' + arch + '.zip'

function onerror (err) {
  throw err
}

var paths = {
  darwin: path.join(__dirname, './dist/Electron.app/Contents/MacOS/Electron'),
  linux: path.join(__dirname, './dist/electron'),
  win32: path.join(__dirname, './dist/electron.exe')
}

var cache = path.join(getHomePath, './.electron')

if (!paths[platform]) throw new Error('Unknown platform: ' + platform)

// use cache if possible
if (pathExists.sync(path.join(cache, filename))) {
  extractFile()
} else {
  mkdir(cache, function(err) {
    if (err) return onerror(err)
    nugget(url, {target: filename, dir: cache, resume: true, verbose: true}, extractFile)
  })
}

function extractFile (err) {
  if (err) return onerror(err)
  fs.writeFileSync(path.join(__dirname, 'path.txt'), paths[platform])
  extract(path.join(cache, filename), {dir: path.join(__dirname, 'dist')}, function (err) {
    if (err) return onerror(err)
  })
}
