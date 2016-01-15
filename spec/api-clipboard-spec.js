var assert, clipboard, nativeImage, path, ref;

assert = require('assert');

path = require('path');

ref = require('electron'), clipboard = ref.clipboard, nativeImage = ref.nativeImage;

describe('clipboard module', function() {
  var fixtures;
  fixtures = path.resolve(__dirname, 'fixtures');
  describe('clipboard.readImage()', function() {
    return it('returns NativeImage intance', function() {
      var i, p;
      p = path.join(fixtures, 'assets', 'logo.png');
      i = nativeImage.createFromPath(p);
      clipboard.writeImage(p);
      return assert.equal(clipboard.readImage().toDataURL(), i.toDataURL());
    });
  });
  describe('clipboard.readText()', function() {
    return it('returns unicode string correctly', function() {
      var text;
      text = '千江有水千江月，万里无云万里天';
      clipboard.writeText(text);
      return assert.equal(clipboard.readText(), text);
    });
  });
  describe('clipboard.readHtml()', function() {
    return it('returns markup correctly', function() {
      var markup, text;
      text = '<string>Hi</string>';
      markup = process.platform === 'darwin' ? '<meta charset=\'utf-8\'><string>Hi</string>' : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><string>Hi</string>' : '<string>Hi</string>';
      clipboard.writeHtml(text);
      return assert.equal(clipboard.readHtml(), markup);
    });
  });
  return describe('clipboard.write()', function() {
    return it('returns data correctly', function() {
      var i, markup, p, text;
      text = 'test';
      p = path.join(fixtures, 'assets', 'logo.png');
      i = nativeImage.createFromPath(p);
      markup = process.platform === 'darwin' ? '<meta charset=\'utf-8\'><b>Hi</b>' : process.platform === 'linux' ? '<meta http-equiv="content-type" ' + 'content="text/html; charset=utf-8"><b>Hi</b>' : '<b>Hi</b>';
      clipboard.write({
        text: "test",
        html: '<b>Hi</b>',
        image: p
      });
      assert.equal(clipboard.readText(), text);
      assert.equal(clipboard.readHtml(), markup);
      return assert.equal(clipboard.readImage().toDataURL(), i.toDataURL());
    });
  });
});
