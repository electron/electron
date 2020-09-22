const { expect } = require('chai');
const path = require('path');
const { Buffer } = require('buffer');

const { clipboard, nativeImage } = require('electron');

describe('clipboard module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');

  // FIXME(zcbenz): Clipboard tests are failing on WOA.
  beforeEach(function () {
    if (process.platform === 'win32' && process.arch === 'arm64') {
      this.skip();
    }
  });

  describe('clipboard.readImage()', () => {
    it('returns NativeImage instance', () => {
      const p = path.join(fixtures, 'assets', 'logo.png');
      const i = nativeImage.createFromPath(p);
      clipboard.writeImage(p);
      const readImage = clipboard.readImage();
      expect(readImage.toDataURL()).to.equal(i.toDataURL());
    });
  });

  describe('clipboard.readText()', () => {
    it('returns unicode string correctly', () => {
      const text = '千江有水千江月，万里无云万里天';
      clipboard.writeText(text);
      expect(clipboard.readText()).to.equal(text);
    });
  });

  describe('clipboard.readHTML()', () => {
    it('returns markup correctly', () => {
      const text = '<string>Hi</string>';
      const markup = process.platform === 'darwin' ? "<meta charset='utf-8'><string>Hi</string>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><string>Hi</string>' : '<string>Hi</string>';
      clipboard.writeHTML(text);
      expect(clipboard.readHTML()).to.equal(markup);
    });
  });

  describe('clipboard.readRTF', () => {
    it('returns rtf text correctly', () => {
      const rtf = '{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}';
      clipboard.writeRTF(rtf);
      expect(clipboard.readRTF()).to.equal(rtf);
    });
  });

  describe('clipboard.readBookmark', () => {
    before(function () {
      if (process.platform === 'linux') {
        this.skip();
      }
    });

    it('returns title and url', () => {
      clipboard.writeBookmark('a title', 'https://electronjs.org');
      expect(clipboard.readBookmark()).to.deep.equal({
        title: 'a title',
        url: 'https://electronjs.org'
      });

      clipboard.writeText('no bookmark');
      expect(clipboard.readBookmark()).to.deep.equal({
        title: '',
        url: ''
      });
    });
  });

  describe('clipboard.write()', () => {
    it('returns data correctly', () => {
      const text = 'test';
      const rtf = '{\\rtf1\\utf8 text}';
      const p = path.join(fixtures, 'assets', 'logo.png');
      const i = nativeImage.createFromPath(p);
      const markup = process.platform === 'darwin' ? "<meta charset='utf-8'><b>Hi</b>" : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><b>Hi</b>' : '<b>Hi</b>';
      const bookmark = { title: 'a title', url: 'test' };
      clipboard.write({
        text: 'test',
        html: '<b>Hi</b>',
        rtf: '{\\rtf1\\utf8 text}',
        bookmark: 'a title',
        image: p
      });

      expect(clipboard.readText()).to.equal(text);
      expect(clipboard.readHTML()).to.equal(markup);
      expect(clipboard.readRTF()).to.equal(rtf);
      const readImage = clipboard.readImage();
      expect(readImage.toDataURL()).to.equal(i.toDataURL());

      if (process.platform !== 'linux') {
        expect(clipboard.readBookmark()).to.deep.equal(bookmark);
      }
    });
  });

  describe('clipboard.read/writeFindText(text)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip();
      }
    });

    it('reads and write text to the find pasteboard', () => {
      clipboard.writeFindText('find this');
      expect(clipboard.readFindText()).to.equal('find this');
    });
  });

  describe('clipboard.readBuffer(format)', () => {
    it('writes a Buffer for the specified format', function () {
      const buffer = Buffer.from('writeBuffer', 'utf8');
      clipboard.writeBuffer('public.utf8-plain-text', buffer);
      expect(buffer.equals(clipboard.readBuffer('public.utf8-plain-text'))).to.equal(true);
    });

    it('throws an error when a non-Buffer is specified', () => {
      expect(() => {
        clipboard.writeBuffer('public.utf8-plain-text', 'hello');
      }).to.throw(/buffer must be a node Buffer/);
    });
  });
});
