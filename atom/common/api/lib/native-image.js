var deprecate, nativeImage;

deprecate = require('electron').deprecate;

nativeImage = process.atomBinding('native_image');


/* Deprecated. */

deprecate.rename(nativeImage, 'createFromDataUrl', 'createFromDataURL');

module.exports = nativeImage;
