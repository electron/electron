#!/usr/bin/env node

var os = require('os')
var path = require('path')
var nugget = require('nugget')
var extract = require('extract-zip')
var fs = require('fs')

var platform = os.platform()
var arch = os.arch()
var version = '0.23.0'
var filename = 'atom-shell-v' + version + '-' + platform + '-' + arch + '.zip'
var url = 'https://github.com/atom/atom-shell/releases/download/v' + version + '/atom-shell-v' + version + '-' + platform + '-' + arch + '.zip'

function onerror (err) {
  throw err
}

var paths = {
  darwin: path.join(__dirname, './dist/Atom.app/Contents/MacOS/Atom'),
  linux: path.join(__dirname, './dist/atom'),
  win32: path.join(__dirname, './dist/atom.exe')
}

var shebang = {
  darwin: '#!/bin/bash\n',
  linux: '#!/bin/bash\n',
  win32: ''
}

var argv = {
  darwin: '"$@"',
  linux: '"$@"',
  win32: '%*' // does this work with " " in the args?
}

if (!paths[platform]) throw new Error('Unknown platform: ' + platform)

nugget(url, {target: filename, dir: __dirname, resume: true, verbose: true}, function (err) {
  if (err) return onerror(err)
  fs.writeFileSync(path.join(__dirname, 'path.txt'), paths[platform])
  fs.writeFileSync(path.join(__dirname, 'run.bat'), shebang[platform] + '"' + paths[platform] + '" ' + argv[platform])
  extract(path.join(__dirname, filename), {dir: path.join(__dirname, 'dist')}, function (err) {
    if (err) return onerror(err)
  })
})
