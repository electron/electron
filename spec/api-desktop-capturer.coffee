assert = require 'assert'
{desktopCapturer} = require 'electron'

describe 'desktopCapturer', ->
  it 'should returns something', (done) ->
    desktopCapturer.getSources {types: ['window', 'screen']}, (error, sources) ->
      assert.equal error, null
      assert.notEqual sources.length, 0
      done()
