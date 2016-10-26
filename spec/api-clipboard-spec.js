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

  describe('clipboard.readBookmark', function () {
    it('returns title and url', function () {
      if (process.platform === 'linux') return

      clipboard.writeBookmark('a title', 'http://electron.atom.io')
      assert.deepEqual(clipboard.readBookmark(), {
        title: 'a title',
        url: 'http://electron.atom.io'
      })

      clipboard.writeText('no bookmark')
      assert.deepEqual(clipboard.readBookmark(), {
        title: '',
        url: ''
      })
    })
  })

  describe('clipboard.write()', function () {
    it('returns data correctly', function () {
      var text = 'test'
      var rtf = '{\\rtf1\\utf8 text}'
      var p = path.join(fixtures, 'assets', 'logo.png')
      var i = nativeImage.createFromPath(p)
      var markup = process.platform === 'darwin' ? "<meta charset='utf-8'><b>Hi</b>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><b>Hi</b>' : '<b>Hi</b>'
      var bookmark = {title: 'a title', url: 'test'}
      clipboard.write({
        text: 'test',
        html: '<b>Hi</b>',
        rtf: '{\\rtf1\\utf8 text}',
        bookmark: 'a title',
        image: p
      })
      assert.equal(clipboard.readText(), text)
      assert.equal(clipboard.readHTML(), markup)
      assert.equal(clipboard.readRTF(), rtf)
      assert.equal(clipboard.readImage().toDataURL(), i.toDataURL())

      if (process.platform !== 'linux') {
        assert.deepEqual(clipboard.readBookmark(), bookmark)
      }
    })
  })

  describe('clipboard.read/writeFindText(text)', function () {
    it('reads and write text to the find pasteboard', function () {
      if (process.platform !== 'darwin') return this.skip()

      clipboard.writeFindText('find this')
      assert.equal(clipboard.readFindText(), 'find this')
    })
  })
})
