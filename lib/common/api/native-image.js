'use strict'

const { deprecate } = require('electron')
const { nativeImage } = process.electronBinding('native_image')

deprecate.fnToProperty(nativeImage, 'templateImage', '_isTemplateImage', '_setTemplateImage')

module.exports = nativeImage
