'use strict';

const assert = require('assert');
const nativeImage = require('electron').nativeImage;
const path = require('path');

describe('nativeImage module', () => {
  describe('createFromPath(path)', () => {
    it('returns an empty image for invalid paths', () => {
      assert(nativeImage.createFromPath('').isEmpty());
      assert(nativeImage.createFromPath('does-not-exist.png').isEmpty());
    });

    it('loads images from paths relative to the current working directory', () => {
      const imagePath = `.${path.sep}${path.join('spec', 'fixtures', 'assets', 'logo.png')}`;
      const image = nativeImage.createFromPath(imagePath);
      assert(!image.isEmpty());
      assert.equal(image.getSize().height, 190);
      assert.equal(image.getSize().width, 538);
    })

    it('loads images from paths with `.` segments', () => {
      const imagePath = `${path.join(__dirname, 'fixtures')}${path.sep}.${path.sep}${path.join('assets', 'logo.png')}`;
      const image = nativeImage.createFromPath(imagePath);
      assert(!image.isEmpty());
      assert.equal(image.getSize().height, 190);
      assert.equal(image.getSize().width, 538);
    });

    it('loads images from paths with `..` segments', () => {
      const imagePath = `${path.join(__dirname, 'fixtures', 'api')}${path.sep}..${path.sep}${path.join('assets', 'logo.png')}`;
      const image = nativeImage.createFromPath(imagePath);
      assert(!image.isEmpty());
      assert.equal(image.getSize().height, 190);
      assert.equal(image.getSize().width, 538);
    });
  });
});
