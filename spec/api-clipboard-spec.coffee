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

  describe 'clipboard.readHtml()', ->
    it 'returns markup correctly', ->
      text = '<string>Hi</string>'
      markup =
        if process.platform is 'darwin'
          '<meta charset=\'utf-8\'><string>Hi</string>'
        else if process.platform is 'linux'
          '<meta http-equiv="content-type" ' +
          'content="text/html; charset=utf-8"><string>Hi</string>'
        else
          '<string>Hi</string>'
      clipboard.writeHtml text
      assert.equal clipboard.readHtml(), markup

  describe 'clipboard.write()', ->
    it 'returns data correctly', ->
      text = 'test'
      p = path.join fixtures, 'assets', 'logo.png'
      i = nativeImage.createFromPath p
      markup =
        if process.platform is 'darwin'
          '<meta charset=\'utf-8\'><b>Hi</b>'
        else if process.platform is 'linux'
          '<meta http-equiv="content-type" ' +
          'content="text/html; charset=utf-8"><b>Hi</b>'
        else
          '<b>Hi</b>'
      clipboard.write {text: "test", html: '<b>Hi</b>', image: p}
      assert.equal clipboard.readText(), text
      assert.equal clipboard.readHtml(), markup
      assert.equal clipboard.readImage().toDataUrl(), i.toDataUrl()
