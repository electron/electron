'use strict'

const assert = require('assert')
const {nativeImage} = require('electron')
const path = require('path')

describe('nativeImage module', () => {
  describe('createEmpty()', () => {
    it('returns an empty image', () => {
      assert(nativeImage.createEmpty().isEmpty())
    })
  })

  describe('createFromBuffer(buffer, scaleFactor)', () => {
    it('returns an empty image when the buffer is empty', () => {
      assert(nativeImage.createFromBuffer(Buffer.from([])).isEmpty())
    })

    it('returns an image created from the given buffer', () => {
      const imageA = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'))

      const imageB = nativeImage.createFromBuffer(imageA.toPNG())
      assert.deepEqual(imageB.getSize(), {width: 538, height: 190})
      assert(imageA.toBitmap().equals(imageB.toBitmap()))

      const imageC = nativeImage.createFromBuffer(imageA.toJPEG(100))
      assert.deepEqual(imageC.getSize(), {width: 538, height: 190})
    })
  })

  describe('createFromDataURL(dataURL)', () => {
    it('returns an empty image when the dataURL is empty', () => {
      assert(nativeImage.createFromDataURL('').isEmpty())
    })

    it('returns an image created from the given buffer', () => {
      const imageA = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'))
      const imageB = nativeImage.createFromDataURL(imageA.toDataURL())
      assert.deepEqual(imageB.getSize(), {width: 538, height: 190})
      assert(imageA.toBitmap().equals(imageB.toBitmap()))
    })
  })

  describe('createFromPath(path)', () => {
    it('returns an empty image for invalid paths', () => {
      assert(nativeImage.createFromPath('').isEmpty())
      assert(nativeImage.createFromPath('does-not-exist.png').isEmpty())
      assert(nativeImage.createFromPath('does-not-exist.ico').isEmpty())
      assert(nativeImage.createFromPath(__dirname).isEmpty())
      assert(nativeImage.createFromPath(__filename).isEmpty())
    })

    it('loads images from paths relative to the current working directory', () => {
      const imagePath = `.${path.sep}${path.join('spec', 'fixtures', 'assets', 'logo.png')}`
      const image = nativeImage.createFromPath(imagePath)
      assert(!image.isEmpty())
      assert.deepEqual(image.getSize(), {width: 538, height: 190})
    })

    it('loads images from paths with `.` segments', () => {
      const imagePath = `${path.join(__dirname, 'fixtures')}${path.sep}.${path.sep}${path.join('assets', 'logo.png')}`
      const image = nativeImage.createFromPath(imagePath)
      assert(!image.isEmpty())
      assert.deepEqual(image.getSize(), {width: 538, height: 190})
    })

    it('loads images from paths with `..` segments', () => {
      const imagePath = `${path.join(__dirname, 'fixtures', 'api')}${path.sep}..${path.sep}${path.join('assets', 'logo.png')}`
      const image = nativeImage.createFromPath(imagePath)
      assert(!image.isEmpty())
      assert.deepEqual(image.getSize(), {width: 538, height: 190})
    })

    it('Gets an NSImage pointer on macOS', () => {
      if (process.platform !== 'darwin') return

      const imagePath = `${path.join(__dirname, 'fixtures', 'api')}${path.sep}..${path.sep}${path.join('assets', 'logo.png')}`
      const image = nativeImage.createFromPath(imagePath)
      const nsimage = image.getNativeHandle()

      assert.equal(nsimage.length, 8)

      // If all bytes are null, that's Bad
      assert.equal(nsimage.reduce((acc, x) => acc || (x !== 0), false), true)
    })

    it('loads images from .ico files on Windows', () => {
      if (process.platform !== 'win32') return

      const imagePath = path.join(__dirname, 'fixtures', 'assets', 'icon.ico')
      const image = nativeImage.createFromPath(imagePath)
      assert(!image.isEmpty())
      assert.deepEqual(image.getSize(), {width: 256, height: 256})
    })
  })

  describe('resize(options)', () => {
    it('returns a resized image', () => {
      const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'))
      assert.deepEqual(image.resize({}).getSize(), {width: 538, height: 190})
      assert.deepEqual(image.resize({width: 269}).getSize(), {width: 269, height: 95})
      assert.deepEqual(image.resize({width: 600}).getSize(), {width: 600, height: 212})
      assert.deepEqual(image.resize({height: 95}).getSize(), {width: 269, height: 95})
      assert.deepEqual(image.resize({height: 200}).getSize(), {width: 566, height: 200})
      assert.deepEqual(image.resize({width: 80, height: 65}).getSize(), {width: 80, height: 65})
      assert.deepEqual(image.resize({width: 600, height: 200}).getSize(), {width: 600, height: 200})
      assert.deepEqual(image.resize({width: 0, height: 0}).getSize(), {width: 0, height: 0})
      assert.deepEqual(image.resize({width: -1, height: -1}).getSize(), {width: 0, height: 0})
    })

    it('returns an empty image when called on an empty image', () => {
      assert(nativeImage.createEmpty().resize({width: 1, height: 1}).isEmpty())
      assert(nativeImage.createEmpty().resize({width: 0, height: 0}).isEmpty())
    })

    it('supports a quality option', () => {
      const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'))
      const good = image.resize({width: 100, height: 100, quality: 'good'})
      const better = image.resize({width: 100, height: 100, quality: 'better'})
      const best = image.resize({width: 100, height: 100, quality: 'best'})
      assert(good.toPNG().length <= better.toPNG().length)
      assert(better.toPNG().length < best.toPNG().length)
    })
  })

  describe('crop(bounds)', () => {
    it('returns an empty image when called on an empty image', () => {
      assert(nativeImage.createEmpty().crop({width: 1, height: 2, x: 0, y: 0}).isEmpty())
      assert(nativeImage.createEmpty().crop({width: 0, height: 0, x: 0, y: 0}).isEmpty())
    })

    it('returns an empty image when the bounds are invalid', () => {
      const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'))
      assert(image.crop({width: 0, height: 0, x: 0, y: 0}).isEmpty())
      assert(image.crop({width: -1, height: 10, x: 0, y: 0}).isEmpty())
      assert(image.crop({width: 10, height: -35, x: 0, y: 0}).isEmpty())
      assert(image.crop({width: 100, height: 100, x: 1000, y: 1000}).isEmpty())
    })

    it('returns a cropped image', () => {
      const image = nativeImage.createFromPath(path.join(__dirname, 'fixtures', 'assets', 'logo.png'))
      const cropA = image.crop({width: 25, height: 64, x: 0, y: 0})
      const cropB = image.crop({width: 25, height: 64, x: 30, y: 40})
      assert.deepEqual(cropA.getSize(), {width: 25, height: 64})
      assert.deepEqual(cropB.getSize(), {width: 25, height: 64})
      assert(!cropA.toPNG().equals(cropB.toPNG()))
    })
  })
})
