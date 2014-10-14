assert = require 'assert'
fs     = require 'fs'
path   = require 'path'
temp   = require 'temp'

describe 'third-party module', ->
  fixtures = path.join __dirname, 'fixtures'
  temp.track()

  describe 'runas', ->
    it 'can be required in renderer', ->
      require 'runas'

    it 'can be required in node binary', (done) ->
      runas = path.join fixtures, 'module', 'runas.js'
      child = require('child_process').fork runas
      child.on 'message', (msg) ->
        assert.equal msg, 'ok'
        done()

  describe 'pathwatcher', ->
    it 'emits file events correctly', (done) ->
      pathwatcher = require 'pathwatcher'
      temp.mkdir 'dir', (err, dir) ->
        return done() if err
        file = path.join dir, 'file'
        fs.writeFileSync file, 'content'
        watcher = pathwatcher.watch file, (event) ->
          assert.equal event, 'change'
          watcher.close()
          done()
        fs.writeFileSync file, 'content2'

  describe 'q', ->
    Q = require 'q'

    describe 'Q.when', ->
      it 'emits the fullfil callback', (done) ->
        Q(true).then (val) ->
          assert.equal val, true
          done()
