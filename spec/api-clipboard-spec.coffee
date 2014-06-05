assert = require 'assert'
clipboard = require 'clipboard'

describe 'clipboard module', ->
  describe 'clipboard.readText()', ->
    it 'returns unicode string correctly', ->
      text = '千江有水千江月，万里无云万里天'
      clipboard.writeText text
      assert.equal clipboard.readText(), text
