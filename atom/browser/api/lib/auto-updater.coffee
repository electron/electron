switch process.platform
  when 'win32'
    autoUpdater = require './auto-updater/auto-updater-win'
  else
    # take the default binding for the current platform
    autoUpdater  = process.atomBinding('auto_updater').autoUpdater
    EventEmitter = require('events').EventEmitter
    autoUpdater.__proto__ = EventEmitter.prototype

autoUpdater.on 'update-downloaded-raw', (args...) ->
  args[3] = new Date(args[3])  # releaseDate
  @emit 'update-downloaded', args..., => @quitAndInstall()

autoUpdater.quitAndInstall = ->
  # If we don't have any window then quitAndInstall immediately.
  BrowserWindow = require 'browser-window'
  windows = BrowserWindow.getAllWindows()
  if windows.length is 0
    @_quitAndInstall()
    return

  # Do the restart after all windows have been closed.
  app = require 'app'
  app.removeAllListeners 'window-all-closed'
  app.once 'window-all-closed', @_quitAndInstall.bind(this)
  win.close() for win in windows

module.exports = autoUpdater
