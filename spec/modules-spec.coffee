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
        watcher = pathwatcher.watch dir, (event) ->
          assert.equal event, 'change'
          watcher.close()
          done()
        fs.writeFile path.join(dir, 'file'), ->
