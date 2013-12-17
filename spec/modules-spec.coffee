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

  describe 'runas', ->
    it 'can be required in renderer', ->
      require 'runas'

    it 'can be required in node binary', (done) ->
      runas = path.join fixtures, 'module', 'runas.js'
      child = require('child_process').fork runas
      child.on 'message', (msg) ->
        assert.equal msg, 'ok'
        done()
