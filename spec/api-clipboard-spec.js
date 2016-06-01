const assert = require('assert')
const path = require('path')

const clipboard = require('electron').clipboard
const nativeImage = require('electron').nativeImage

describe('clipboard module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')

  describe('clipboard.readImage()', function () {
    it('returns NativeImage intance', function () {
      var p = path.join(fixtures, 'assets', 'logo.png')
      var i = nativeImage.createFromPath(p)
      clipboard.writeImage(p)
      assert.equal(clipboard.readImage().toDataURL(), i.toDataURL())
    })
  })

  describe('clipboard.readText()', function () {
    it('returns unicode string correctly', function () {
      var text = '千江有水千江月，万里无云万里天'
      clipboard.writeText(text)
      assert.equal(clipboard.readText(), text)
    })
  })

  describe('clipboard.readHTML()', function () {
    it('returns markup correctly', function () {
      var text = '<string>Hi</string>'
      var markup = process.platform === 'darwin' ? "<meta charset='utf-8'><string>Hi</string>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><string>Hi</string>' : '<string>Hi</string>'
      clipboard.writeHTML(text)
      assert.equal(clipboard.readHTML(), markup)
    })
  })

  describe('clipboard.readRTF', function () {
    it('returns rtf text correctly', function () {
      var rtf = '{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}'
      clipboard.writeRTF(rtf)
      assert.equal(clipboard.readRTF(), rtf)
    })
  })

  describe('clipboard.write()', function () {
    it('returns data correctly', function () {
      var text = 'test'
      var rtf = '{\\rtf1\\utf8 text}'
      var p = path.join(fixtures, 'assets', 'logo.png')
      var i = nativeImage.createFromPath(p)
      var markup = process.platform === 'darwin' ? "<meta charset='utf-8'><b>Hi</b>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><b>Hi</b>' : '<b>Hi</b>'
      clipboard.write({
        text: 'test',
        html: '<b>Hi</b>',
        rtf: '{\\rtf1\\utf8 text}',
        image: p
      })
      assert.equal(clipboard.readText(), text)
      assert.equal(clipboard.readHTML(), markup)
      assert.equal(clipboard.readRTF(), rtf)
      assert.equal(clipboard.readImage().toDataURL(), i.toDataURL())
    })
  })
})
