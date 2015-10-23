{EventEmitter} = require 'events'
{autoUpdater} = process.atomBinding 'auto_updater'

autoUpdater.__proto__ = EventEmitter.prototype

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
