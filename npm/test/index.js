var tape = require('tape')
var electron = require('../')
var path = require('path');
var pathExists = require('path-exists')
var getHomePath = require('home-path')()

tape('has local binary', function(t) {
  t.ok(pathExists.sync(electron), 'electron was downloaded')
  t.end()
})

tape('has cache folder', function(t) {
  t.ok(pathExists.sync(path.join(getHomePath, './.electron')), 'cache exists')
  t.end()
})
