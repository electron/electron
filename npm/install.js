#!/usr/bin/env node

var os = require('os')
var path = require('path')
var nugget = require('nugget')
var extract = require('extract-zip')

var version = '0.18.0'
var name = version+'-atom-shell.zip'
var url = 'https://github.com/atom/atom-shell/releases/download/v'+version+'/atom-shell-v'+version+'-'+os.platform()+'-'+os.arch()+'.zip'

var onerror = function(err) {
  throw err
}

nugget(url, {target:name, dir:__dirname, resume:true}, function(err) {
  if (err) return onerror(err)
  extract(path.join(__dirname, name), {dir:path.join(__dirname, os.platform())}, function(err) {
    if (err) return onerror(err)
  })
})
