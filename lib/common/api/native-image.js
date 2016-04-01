const deprecate = require('electron').deprecate
const nativeImage = process.atomBinding('native_image')

// Deprecated.
deprecate.rename(nativeImage, 'createFromDataUrl', 'createFromDataURL')

module.exports = nativeImage
