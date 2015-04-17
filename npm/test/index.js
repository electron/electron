var tape = require('tape')
var electron = require('../')
var fs = require('fs')

tape('has binary', function(t) {
  t.ok(fs.existsSync(electron), 'electron was downloaded')
  t.end()
})