assert = require 'assert'
path = require 'path'

{remote} = require 'electron'
{app, autoUpdater, BrowserWindow} = remote.require 'electron'

describe 'autoUpdater module', ->
  afterEach ->
    remote.getCurrentWindow().webContents.executeJavaScript 'window.onbeforeunload = function() {};'
    autoUpdater.removeAllListeners()
    app.removeAllListeners()

  describe 'autoUpdater.quitAndInstall()', ->
    # Fails: `quitAndInstall` will cause the app to quit and then it will not reopen.
    xit 'should emit an error if called before \'update-downloaded\' is emitted', (done) ->
      autoUpdater.on 'error', ->
        done()
      autoUpdater.quitAndInstall()

    it 'should not quit if all windows do not close', ->
      remote.getCurrentWindow().webContents.executeJavaScript 'window.onbeforeunload = function() { return false; }'
      autoUpdater.quitAndInstall()

    # At this point it would be nicer to mock out `auto_updater::AutoUpdater::QuitAndInstall`
    # so that we could test the entire shutdown flow (including windows closing)
    # up until that point, but I (wearhere) don't know how to do that, so we
    # still abort the quit process by preventing the window from closing.
    it 'should emit before-quit event', (done) ->
      remote.getCurrentWindow().webContents.executeJavaScript 'window.onbeforeunload = function() { return false; }'
      app.on 'before-quit', ->
        done()
      autoUpdater.quitAndInstall()