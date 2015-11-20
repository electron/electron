switch process.platform
  when 'win32'
    module.exports = require './auto-updater/auto-updater-win'
  when 'darwin'
    module.exports = require './auto-updater/auto-updater-mac'
  else
    throw new Error('auto-updater is not implemented on this platform')
