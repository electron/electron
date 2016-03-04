const assert = require('assert');
const nativeImage = require('electron').nativeImage;
const path = require('path');

describe('nativeImage module', function () {
  describe('createFromPath(path)', function () {
    it('normalizes paths', function () {
      const nonAbsolutePath = path.join(__dirname, 'fixtures', 'api') + path.sep + '..' + path.sep + path.join('assets', 'logo.png');
      const image = nativeImage.createFromPath(nonAbsolutePath);
      assert(!image.isEmpty());
    });
  });
});
