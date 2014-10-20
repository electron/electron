#!/usr/bin/env node

var os = require('os')
var path = require('path')
var nugget = require('nugget')
var extract = require('extract-zip')
var fs = require('fs')

var platform = os.platform()
var arch = os.arch()
var version = '0.18.0'
var name = 'atom-shell-v'+version+'-'+platform+'-'+arch+'.zip'
var url = 'https://github.com/atom/atom-shell/releases/download/v'+version+'/atom-shell-v'+version+'-'+platform+'-'+arch+'.zip'

var onerror = function(err) {
  throw err
}

var paths = {
  darwin: path.join(__dirname, './dist/Atom.app/Contents/MacOS/Atom'),
  linux: path.join(__dirname, './dist/atom'),
  win32: path.join(__dirname, './dist/atom.exe')
}

if (!paths[platform]) throw new Error('Unknown platform: '+platform)

nugget(url, {target:name, dir:__dirname, resume:true}, function(err) {
  if (err) return onerror(err)
  fs.writeFileSync(path.join(__dirname, 'run.bat'), '"'+paths[platform]+'"')
  extract(path.join(__dirname, name), {dir:path.join(__dirname, 'dist')}, function(err) {
    if (err) return onerror(err)
  })
})
