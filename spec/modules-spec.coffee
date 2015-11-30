assert = require 'assert'
fs     = require 'fs'
path   = require 'path'
temp   = require 'temp'

describe 'third-party module', ->
  fixtures = path.join __dirname, 'fixtures'
  temp.track()

  # If the test is executed with the debug build on Windows, we will skip it
  # because native modules don't work with the debug build (see issue #2558).
  if process.platform isnt 'win32' or
      process.execPath.toLowerCase().indexOf('\\out\\d\\') is -1
    describe 'runas', ->
      it 'can be required in renderer', ->
        require 'runas'

      it 'can be required in node binary', (done) ->
        runas = path.join fixtures, 'module', 'runas.js'
        child = require('child_process').fork runas
        child.on 'message', (msg) ->
          assert.equal msg, 'ok'
          done()

    describe 'ffi', ->
      it 'does not crash', ->
        ffi = require 'ffi'
        libm = ffi.Library('libm', ceil: [ 'double', [ 'double' ] ])
        assert.equal libm.ceil(1.5), 2

  describe 'q', ->
    Q = require 'q'

    describe 'Q.when', ->
      it 'emits the fullfil callback', (done) ->
        Q(true).then (val) ->
          assert.equal val, true
          done()
