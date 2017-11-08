'use strict'

const assert = require('assert')
const {nativeImage} = require('electron')
const path = require('path')

describe('nativeImage module', () => {
  const ImageFormat = {
    PNG: 'png',
    JPEG: 'jpeg'
  }

  const images = [
    {
      filename: 'logo.png',
      format: ImageFormat.PNG,
      hasAlphaChannel: true,
      hasDataUrl: false,
      width: 538,
      height: 190
    },
    {
      dataUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAAC0lEQVQYlWNgAAIAAAUAAdafFs0AAAAASUVORK5CYII=',
      filename: '1x1.png',
      format: ImageFormat.PNG,
      hasAlphaChannel: true,
      hasDataUrl: true,
      height: 1,
      width: 1
    },
    {
      dataUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAFUlEQVQYlWP8////fwYGBgYmBigAAD34BABBrq9BAAAAAElFTkSuQmCC',
      filename: '2x2.jpg',
      format: ImageFormat.JPEG,
      hasAlphaChannel: false,
      hasDataUrl: true,
      height: 2,
      width: 2
    },
    {
      dataUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAMAAAADCAYAAABWKLW/AAAADElEQVQYlWNgIAoAAAAnAAGZWEMnAAAAAElFTkSuQmCC',
      filename: '3x3.png',
      format: ImageFormat.PNG,
      hasAlphaChannel: true,
      hasDataUrl: true,
      height: 3,
      width: 3
    }
  ]

  /**
   * @param {?string} filename
   * @returns {?string} Full path.
   */
  const getImagePathFromFilename = (filename) => {
    return (filename === null) ? null
        : path.join(__dirname, 'fixtures', 'assets', filename)
  }

  /**
   * @param {!Object} image
   * @param {Object} filters
   * @returns {boolean}
   */
  const imageMatchesTheFilters = (image, filters = null) => {
    if (filters === null) {
      return true
    }

    return Object.entries(filters)
        .every(([key, value]) => image[key] === value)
  }

  /**
   * @param {!Object} filters
   * @returns {!Array} A matching images list.
   */
  const getImages = (filters) => {
    const matchingImages = images
        .filter(i => imageMatchesTheFilters(i, filters))

    // Add `.path` property to every image.
    matchingImages
        .forEach(i => { i.path = getImagePathFromFilename(i.filename) })

    return matchingImages
  }

  /**
   * @param {!Object} filters
   * @returns {Object} A matching image if any.
   */
  const getImage = (filters) => {
    const matchingImages = getImages(filters)

    let matchingImage = null
    if (matchingImages.length > 0) {
      matchingImage = matchingImages[0]
    }

    return matchingImage
  }

  describe('createEmpty()', () => {
    it('returns an empty image', () => {
      const empty = nativeImage.createEmpty()
      assert.equal(empty.isEmpty(), true)
      assert.equal(empty.getAspectRatio(), 1)
      assert.equal(empty.toDataURL(), 'data:image/png;base64,')
      assert.equal(empty.toDataURL({scaleFactor: 2.0}), 'data:image/png;base64,')
      assert.deepEqual(empty.getSize(), {width: 0, height: 0})
      assert.deepEqual(empty.getBitmap(), [])
      assert.deepEqual(empty.getBitmap({scaleFactor: 2.0}), [])
      assert.deepEqual(empty.toBitmap(), [])
      assert.deepEqual(empty.toBitmap({scaleFactor: 2.0}), [])
      assert.deepEqual(empty.toJPEG(100), [])
      assert.deepEqual(empty.toPNG(), [])
      assert.deepEqual(empty.toPNG({scaleFactor: 2.0}), [])

      if (process.platform === 'darwin') {
        assert.deepEqual(empty.getNativeHandle(), [])
      }
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

      const imageD = nativeImage.createFromBuffer(imageA.toBitmap(),
        {width: 538, height: 190})
      assert.deepEqual(imageD.getSize(), {width: 538, height: 190})

      const imageE = nativeImage.createFromBuffer(imageA.toBitmap(),
        {width: 100, height: 200})
      assert.deepEqual(imageE.getSize(), {width: 100, height: 200})

      const imageF = nativeImage.createFromBuffer(imageA.toBitmap())
      assert(imageF.isEmpty())

      const imageG = nativeImage.createFromBuffer(imageA.toPNG(),
        {width: 100, height: 200})
      assert.deepEqual(imageG.getSize(), {width: 538, height: 190})

      const imageH = nativeImage.createFromBuffer(imageA.toJPEG(100),
        {width: 100, height: 200})
      assert.deepEqual(imageH.getSize(), {width: 538, height: 190})

      const imageI = nativeImage.createFromBuffer(imageA.toBitmap(),
        {width: 538, height: 190, scaleFactor: 2.0})
      assert.deepEqual(imageI.getSize(), {width: 269, height: 95})

      const imageJ = nativeImage.createFromBuffer(imageA.toPNG(), 2.0)
      assert.deepEqual(imageJ.getSize(), {width: 269, height: 95})
    })
  })

  describe('createFromDataURL(dataURL)', () => {
    it('returns an empty image from the empty string', () => {
      assert(nativeImage.createFromDataURL('').isEmpty())
    })

    it('returns an image created from the given string', () => {
      const imagesData = getImages({hasDataUrl: true})
      for (const imageData of imagesData) {
        const imageFromPath = nativeImage.createFromPath(imageData.path)
        const imageFromDataUrl = nativeImage.createFromDataURL(imageData.dataUrl)

        assert(!imageFromDataUrl.isEmpty())
        assert.deepEqual(imageFromDataUrl.getSize(), imageFromPath.getSize())
        assert(imageFromPath.toBitmap().equals(imageFromDataUrl.toBitmap()))
      }
    })
  })

  describe('toDataURL()', () => {
    it('returns a PNG data URL', () => {
      const imagesData = getImages({hasDataUrl: true})
      for (const imageData of imagesData) {
        const imageFromPath = nativeImage.createFromPath(imageData.path)

        assert.equal(imageFromPath.toDataURL(), imageData.dataUrl)
        assert.equal(imageFromPath.toDataURL({scaleFactor: 2.0}), imageData.dataUrl)
      }
    })

    it('returns a data URL at 1x scale factor by default', () => {
      const imageData = getImage({filename: 'logo.png'})
      const image = nativeImage.createFromPath(imageData.path)

      const imageOne = nativeImage.createFromBuffer(image.toPNG(), {
        width: image.getSize().width,
        height: image.getSize().height,
        scaleFactor: 2.0
      })
      assert.deepEqual(imageOne.getSize(),
          {width: imageData.width / 2, height: imageData.height / 2})

      const imageTwo = nativeImage.createFromDataURL(imageOne.toDataURL())
      assert.deepEqual(imageTwo.getSize(),
          {width: imageData.width, height: imageData.height})

      assert(imageOne.toBitmap().equals(imageTwo.toBitmap()))
    })

    it('supports a scale factor', () => {
      const imageData = getImage({filename: 'logo.png'})
      const image = nativeImage.createFromPath(imageData.path)
      const expectedSize = {width: imageData.width, height: imageData.height}

      const imageFromDataUrlOne = nativeImage.createFromDataURL(
          image.toDataURL({scaleFactor: 1.0}))
      assert.deepEqual(imageFromDataUrlOne.getSize(), expectedSize)

      const imageFromDataUrlTwo = nativeImage.createFromDataURL(
          image.toDataURL({scaleFactor: 2.0}))
      assert.deepEqual(imageFromDataUrlTwo.getSize(), expectedSize)
    })
  })

  describe('toPNG()', () => {
    it('returns a buffer at 1x scale factor by default', () => {
      const imageData = getImage({filename: 'logo.png'})
      const imageA = nativeImage.createFromPath(imageData.path)

      const imageB = nativeImage.createFromBuffer(imageA.toPNG(), {
        width: imageA.getSize().width,
        height: imageA.getSize().height,
        scaleFactor: 2.0
      })
      assert.deepEqual(imageB.getSize(),
          {width: imageData.width / 2, height: imageData.height / 2})

      const imageC = nativeImage.createFromBuffer(imageB.toPNG())
      assert.deepEqual(imageC.getSize(),
          {width: imageData.width, height: imageData.height})

      assert(imageB.toBitmap().equals(imageC.toBitmap()))
    })

    it('supports a scale factor', () => {
      const imageData = getImage({filename: 'logo.png'})
      const image = nativeImage.createFromPath(imageData.path)

      const imageFromBufferOne = nativeImage.createFromBuffer(
          image.toPNG({scaleFactor: 1.0}))
      assert.deepEqual(
          imageFromBufferOne.getSize(),
          {width: imageData.width, height: imageData.height})

      const imageFromBufferTwo = nativeImage.createFromBuffer(
          image.toPNG({scaleFactor: 2.0}), {scaleFactor: 2.0})
      assert.deepEqual(
          imageFromBufferTwo.getSize(),
          {width: imageData.width / 2, height: imageData.height / 2})
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

  describe('createFromNamedImage(name)', () => {
    it('returns empty for invalid options', () => {
      const image = nativeImage.createFromNamedImage('totally_not_real')
      assert(image.isEmpty())
    })

    it('returns empty on non-darwin platforms', () => {
      if (process.platform === 'darwin') return

      const image = nativeImage.createFromNamedImage('NSActionTemplate')
      assert(image.isEmpty())
    })

    it('returns a valid image on darwin', () => {
      if (process.platform !== 'darwin') return

      const image = nativeImage.createFromNamedImage('NSActionTemplate')
      assert(!image.isEmpty())
    })

    it('returns allows an HSL shift for a valid image on darwin', () => {
      if (process.platform !== 'darwin') return

      const image = nativeImage.createFromNamedImage('NSActionTemplate', [0.5, 0.2, 0.8])
      assert(!image.isEmpty())
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

  describe('getAspectRatio()', () => {
    it('returns an aspect ratio of an empty image', () => {
      assert.equal(nativeImage.createEmpty().getAspectRatio(), 1.0)
    })

    it('returns an aspect ratio of an image', () => {
      const imageData = getImage({filename: 'logo.png'})
      // imageData.width / imageData.height = 2.831578947368421
      const expectedAspectRatio = 2.8315789699554443

      const image = nativeImage.createFromPath(imageData.path)
      assert.equal(image.getAspectRatio(), expectedAspectRatio)
    })
  })

  describe('addRepresentation()', () => {
    it('supports adding a buffer representation for a scale factor', () => {
      const image = nativeImage.createEmpty()

      const imageDataOne = getImage({width: 1, height: 1})
      image.addRepresentation({
        scaleFactor: 1.0,
        buffer: nativeImage.createFromPath(imageDataOne.path).toPNG()
      })

      const imageDataTwo = getImage({width: 2, height: 2})
      image.addRepresentation({
        scaleFactor: 2.0,
        buffer: nativeImage.createFromPath(imageDataTwo.path).toPNG()
      })

      const imageDataThree = getImage({width: 3, height: 3})
      image.addRepresentation({
        scaleFactor: 3.0,
        buffer: nativeImage.createFromPath(imageDataThree.path).toPNG()
      })

      image.addRepresentation({
        scaleFactor: 4.0,
        buffer: 'invalid'
      })

      assert.equal(image.isEmpty(), false)
      assert.deepEqual(image.getSize(), {width: 1, height: 1})

      assert.equal(image.toDataURL({scaleFactor: 1.0}), imageDataOne.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 2.0}), imageDataTwo.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 3.0}), imageDataThree.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 4.0}), imageDataThree.dataUrl)
    })

    it('supports adding a data URL representation for a scale factor', () => {
      const image = nativeImage.createEmpty()

      const imageDataOne = getImage({width: 1, height: 1})
      image.addRepresentation({
        scaleFactor: 1.0,
        dataURL: imageDataOne.dataUrl
      })

      const imageDataTwo = getImage({width: 2, height: 2})
      image.addRepresentation({
        scaleFactor: 2.0,
        dataURL: imageDataTwo.dataUrl
      })

      const imageDataThree = getImage({width: 3, height: 3})
      image.addRepresentation({
        scaleFactor: 3.0,
        dataURL: imageDataThree.dataUrl
      })

      image.addRepresentation({
        scaleFactor: 4.0,
        dataURL: 'invalid'
      })

      assert.equal(image.isEmpty(), false)
      assert.deepEqual(image.getSize(), {width: 1, height: 1})

      assert.equal(image.toDataURL({scaleFactor: 1.0}), imageDataOne.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 2.0}), imageDataTwo.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 3.0}), imageDataThree.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 4.0}), imageDataThree.dataUrl)
    })

    it('supports adding a representation to an existing image', () => {
      const imageDataOne = getImage({width: 1, height: 1})
      const image = nativeImage.createFromPath(imageDataOne.path)

      const imageDataTwo = getImage({width: 2, height: 2})
      image.addRepresentation({
        scaleFactor: 2.0,
        dataURL: imageDataTwo.dataUrl
      })

      const imageDataThree = getImage({width: 3, height: 3})
      image.addRepresentation({
        scaleFactor: 2.0,
        dataURL: imageDataThree.dataUrl
      })

      assert.equal(image.toDataURL({scaleFactor: 1.0}), imageDataOne.dataUrl)
      assert.equal(image.toDataURL({scaleFactor: 2.0}), imageDataTwo.dataUrl)
    })
  })
})
