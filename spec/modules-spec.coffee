assert = require 'assert'
fs     = require 'fs'
path   = require 'path'

describe 'third-party module', ->
  fixtures = path.join __dirname, 'fixtures'

  describe 'runas', ->
    it 'can be required in renderer', ->
      require 'runas'

    it 'can be required in node binary', (done) ->
      runas = path.join fixtures, 'module', 'runas.js'
      child = require('child_process').fork runas
      child.on 'message', (msg) ->
        assert.equal msg, 'ok'
        done()
