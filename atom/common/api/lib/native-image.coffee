{deprecate} = require 'electron'
nativeImage = process.atomBinding 'native_image'

### Deprecated. ###
deprecate.rename nativeImage, 'createFromDataUrl', 'createFromDataURL'

module.exports = nativeImage
