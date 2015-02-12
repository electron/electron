if process.platform is 'linux' and process.type is 'renderer'
  # On Linux we could not access clipboard in renderer process.
  module.exports = require('remote').require 'clipboard'
else
  binding = process.atomBinding 'clipboard'

  module.exports =
    has: (format, type='standard') -> binding._has format, type
    read: (format, type='standard') -> binding._read format, type
    readText: (type='standard') -> binding._readText type
    writeText: (text, type='standard') -> binding._writeText text, type
    readImage: (type='standard') -> binding._readImage type
    clear: (type='standard') -> binding._clear type
