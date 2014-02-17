AutoUpdater = process.atomBinding('auto_updater').AutoUpdater
EventEmitter = require('events').EventEmitter

AutoUpdater::__proto__ = EventEmitter.prototype

autoUpdater = new AutoUpdater
autoUpdater.on 'update-downloaded-raw', (args...) ->
  args[3] = new Date(args[3])  # releaseDate
  @emit 'update-downloaded', args..., => @quitAndInstall()

autoUpdater.quitAndInstall = ->
  # Do the restart after all windows have been closed.
  app = require 'app'
  app.removeAllListeners 'window-all-closed'
  app.once 'window-all-closed', AutoUpdater::quitAndInstall.bind(this)

  # Tell all windows to remove beforeunload handler and then close itself.
  ipc = require 'ipc'
  BrowserWindow = require 'browser-window'
  ipc.sendChannel win.getProcessId(), win.getRoutingId(), 'ATOM_SHELL_SILENT_CLOSE' for win in BrowserWindow.getAllWindows()

module.exports = autoUpdater
