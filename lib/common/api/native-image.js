const {deprecate} = require('electron')

const nativeImage = process.atomBinding('native_image')

nativeImage.toPng = function () {
  deprecate.warn('nativeImage.toPng()', 'nativeImage.toPNG()')
  return nativeImage.toPNG
}

nativeImage.toJpeg = function () {
  deprecate.warn('nativeImage.toJpeg()', 'nativeImage.toJPEG()')
  return nativeImage.toJPEG
}

module.exports = nativeImage
