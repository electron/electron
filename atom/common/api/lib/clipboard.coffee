if process.platform is 'linux' and process.type is 'renderer'
  # On Linux we could not access clipboard in renderer process.
  module.exports = require('remote').require 'clipboard'
else
  binding = process.atomBinding 'clipboard'

  module.exports =
    availableFormats: (type='standard') -> binding._availableFormats type
    has: (format, type='standard') -> binding._has format, type
    read: (format, type='standard') -> binding._read format, type
    readText: (type='standard') -> binding._readText type
    writeText: (text, type='standard') -> binding._writeText text, type
    readHtml: (type='standard') -> binding._readHtml type
    writeHtml: (markup, type='standard') -> binding._writeHtml markup, type
    readImage: (type='standard') -> binding._readImage type
    writeImage: (image, type='standard') -> binding._writeImage image, type
    clear: (type='standard') -> binding._clear type
