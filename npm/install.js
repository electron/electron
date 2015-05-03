#!/usr/bin/env node

var os = require('os')
var path = require('path')
var nugget = require('nugget')
var extract = require('extract-zip')
var fs = require('fs')

var platform = os.platform()
var arch = os.arch()
var version = '0.25.2'
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

if (!paths[platform]) throw new Error('Unknown platform: ' + platform)

nugget(url, {target: filename, dir: __dirname, resume: true, verbose: true}, function (err) {
  if (err) return onerror(err)
  fs.writeFileSync(path.join(__dirname, 'path.txt'), paths[platform])
  extract(path.join(__dirname, filename), {dir: path.join(__dirname, 'dist')}, function (err) {
    if (err) return onerror(err)
  })
})
