'use strict'

const assert = require('assert')
const {nativeImage} = require('electron')
const path = require('path')

describe('nativeImage module', () => {
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
      assert.deepEqual(image.resize({width: 400}).getSize(), {width: 400, height: 190})
      assert.deepEqual(image.resize({height: 123}).getSize(), {width: 538, height: 123})
      assert.deepEqual(image.resize({width: 80, height: 65}).getSize(), {width: 80, height: 65})
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
})
