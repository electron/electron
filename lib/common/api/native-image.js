const {deprecate} = require('electron')
const nativeImage = process.atomBinding('native_image')

// TODO(codebytere): remove in 3.0
nativeImage.prototype.toJpeg = function () {
  if (!process.noDeprecations) {
    deprecate.warn('nativeImage.toJpeg()', 'nativeImage.toJPEG()')
  }
  return nativeImage.toJPEG()
}

// TODO(codebytere): remove in 3.0
nativeImage.prototype.toPng = function () {
  if (!process.noDeprecations) {
    deprecate.warn('nativeImage.toPng()', 'nativeImage.toPNG()')
  }
  return nativeImage.toPNG()
}

module.exports = nativeImage
