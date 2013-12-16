assert = require 'assert'
fs     = require 'fs'
path   = require 'path'

describe 'third-party module', ->
  fixtures = path.join __dirname, 'fixtures'

  xdescribe 'unzip', ->
    unzip = require 'unzip'

    it 'fires close event', (done) ->
      fs.createReadStream(path.join(fixtures, 'zip', 'a.zip'))
        .pipe(unzip.Parse())
        .on('close', done)

  describe 'time', ->
    it 'can be required in renderer', ->
      time = require 'time'
      now = new time.Date()
      now.setTimezone 'America/Los_Angeles'
      assert.equal now.getTimezone(), 'America/Los_Angeles'

    it 'can be required in node binary', (done) ->
      time = path.join fixtures, 'module', 'time.js'
      child = require('child_process').fork time
      child.on 'message', (msg) ->
        assert.equal msg, 'ok'
        done()
