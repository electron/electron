const assert = require('assert')
const path = require('path')
const {Buffer} = require('buffer')

const {clipboard, nativeImage} = require('electron')

describe('clipboard module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')

  describe('clipboard.readImage()', () => {
    it('returns NativeImage instance', () => {
      const p = path.join(fixtures, 'assets', 'logo.png')
      const i = nativeImage.createFromPath(p)
      clipboard.writeImage(p)
      assert.equal(clipboard.readImage().toDataURL(), i.toDataURL())
    })
  })

  describe('clipboard.readText()', () => {
    it('returns unicode string correctly', () => {
      const text = '千江有水千江月，万里无云万里天'
      clipboard.writeText(text)
      assert.equal(clipboard.readText(), text)
    })
  })

  describe('clipboard.readHTML()', () => {
    it('returns markup correctly', () => {
      const text = '<string>Hi</string>'
      const markup = process.platform === 'darwin' ? "<meta charset='utf-8'><string>Hi</string>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><string>Hi</string>' : '<string>Hi</string>'
      clipboard.writeHTML(text)
      assert.equal(clipboard.readHTML(), markup)
    })
  })

  describe('clipboard.readRTF', () => {
    it('returns rtf text correctly', () => {
      const rtf = '{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}'
      clipboard.writeRTF(rtf)
      assert.equal(clipboard.readRTF(), rtf)
    })
  })

  describe('clipboard.readBookmark', () => {
    before(function () {
      if (process.platform === 'linux') {
        this.skip()
      }
    })

    it('returns title and url', () => {
      clipboard.writeBookmark('a title', 'https://electronjs.org')
      assert.deepEqual(clipboard.readBookmark(), {
        title: 'a title',
        url: 'https://electronjs.org'
      })

      clipboard.writeText('no bookmark')
      assert.deepEqual(clipboard.readBookmark(), {
        title: '',
        url: ''
      })
    })
  })

  describe('clipboard.write()', () => {
    it('returns data correctly', () => {
      const text = 'test'
      const rtf = '{\\rtf1\\utf8 text}'
      const p = path.join(fixtures, 'assets', 'logo.png')
      const i = nativeImage.createFromPath(p)
      const markup = process.platform === 'darwin' ? "<meta charset='utf-8'><b>Hi</b>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><b>Hi</b>' : '<b>Hi</b>'
      const bookmark = {title: 'a title', url: 'test'}
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

  describe('clipboard.read/writeFindText(text)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('reads and write text to the find pasteboard', () => {
      clipboard.writeFindText('find this')
      assert.equal(clipboard.readFindText(), 'find this')
    })
  })

  describe('clipboard.writeBuffer(format, buffer)', () => {
    it('writes a Buffer for the specified format', function () {
      if (process.platform !== 'darwin') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      const buffer = Buffer.from('writeBuffer', 'utf8')
      clipboard.writeBuffer('public.utf8-plain-text', buffer)
      assert.equal(clipboard.readText(), 'writeBuffer')
    })

    it('throws an error when a non-Buffer is specified', () => {
      assert.throws(() => {
        clipboard.writeBuffer('public.utf8-plain-text', 'hello')
      }, /buffer must be a node Buffer/)
    })
  })

  describe('clipboard.readBuffer(format)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('returns a Buffer of the content for the specified format', () => {
      const buffer = Buffer.from('this is binary', 'utf8')
      clipboard.writeText(buffer.toString())
      assert(buffer.equals(clipboard.readBuffer('public.utf8-plain-text')))
    })
  })
})
