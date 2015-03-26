assert = require 'assert'
clipboard = require 'clipboard'
nativeImage = require 'native-image'
path = require 'path'

describe 'clipboard module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  describe 'clipboard.readImage()', ->
    it 'returns NativeImage intance', ->
      p = path.join fixtures, 'assets', 'logo.png'
      i = nativeImage.createFromPath p
      clipboard.writeImage p
      assert.equal clipboard.readImage().toDataUrl(), i.toDataUrl()

  describe 'clipboard.readText()', ->
    it 'returns unicode string correctly', ->
      text = '千江有水千江月，万里无云万里天'
      clipboard.writeText text
      assert.equal clipboard.readText(), text
