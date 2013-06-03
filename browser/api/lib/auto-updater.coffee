AutoUpdater = process.atomBinding('auto_updater').AutoUpdater
EventEmitter = require('events').EventEmitter

AutoUpdater::__proto__ = EventEmitter.prototype

autoUpdater = new AutoUpdater
autoUpdater.on 'will-install-update-raw', (event, version) ->
  @emit 'will-install-update', event, version, => @continueUpdate()

module.exports = autoUpdater
