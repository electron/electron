'use strict'

const { deprecate } = require('electron')
const { NativeImage, nativeImage } = process.electronBinding('native_image')

deprecate.fnToProperty(NativeImage.prototype, 'isMacTemplateImage', '_isTemplateImage', '_setTemplateImage')

module.exports = nativeImage
