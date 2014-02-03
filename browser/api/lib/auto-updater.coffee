AutoUpdater = process.atomBinding('auto_updater').AutoUpdater
EventEmitter = require('events').EventEmitter

AutoUpdater::__proto__ = EventEmitter.prototype

autoUpdater = new AutoUpdater
autoUpdater.on 'update-downloaded-raw', (args...) ->
  args[3] = new Date(args[3])  # releaseDate
  @emit 'update-downloaded', args..., => @quitAndInstall()

module.exports = autoUpdater
