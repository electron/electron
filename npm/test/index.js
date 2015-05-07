var tape = require('tape')
var electron = require('../')
var pathExists = require('path-exists')

tape('has binary', function(t) {
  t.ok(pathExists.sync(electron), 'electron was downloaded')
  t.end()
})
