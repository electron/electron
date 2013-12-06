assert = require 'assert'
fs     = require 'fs'
path   = require 'path'

describe 'third-party modules', ->
  fixtures = path.join __dirname, 'fixtures'

  describe 'd3', ->
    it 'can be required', (done) ->
      require 'd3'
      done()

  describe 'unzip', ->
    unzip = require 'unzip'

    it 'fires close event', (done) ->
      fs.createReadStream(path.join(fixtures, 'zip', 'a.zip'))
        .pipe(unzip.Parse())
        .on('close', done)

  describe 'int64-native', ->
    it 'can be required in renderer', ->
      Int64 = require 'int64-native'
      x = new Int64()
      y = new Int64(42)
      z = new Int64(0xfedcba98, 0x76543210)
      w = new Int64('fedcba9876543210')
      assert.equal x.toString(), '0000000000000000'
      assert.equal y.toString(), '000000000000002a'
      assert.equal z.toString(), 'fedcba9876543210'
      assert.equal w.toString(), 'fedcba9876543210'

    it 'can be required in node binary', (done) ->
      int64_native = path.join fixtures, 'module', 'int64_native.js'
      child = require('child_process').fork int64_native
      child.on 'message', (msg) ->
        assert.equal msg, 'ok'
        done()
