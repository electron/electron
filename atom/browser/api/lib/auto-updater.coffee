{deprecate} = require 'electron'

autoUpdater =
  if process.platform is 'win32'
    require './auto-updater/auto-updater-win'
  else
    require './auto-updater/auto-updater-native'

### Deprecated. ###
deprecate.rename autoUpdater, 'setFeedUrl', 'setFeedURL'

module.exports = autoUpdater
