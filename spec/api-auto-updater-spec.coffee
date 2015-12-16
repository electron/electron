assert = require 'assert'
path = require 'path'

{remote} = require 'electron'
{app, autoUpdater, BrowserWindow} = remote.require 'electron'

describe 'autoUpdater module', ->
  afterEach ->
    remote.getCurrentWindow().webContents.executeJavaScript 'window.onbeforeunload = function() {};'
    autoUpdater.removeAllListeners()

  describe 'autoUpdater.quitAndInstall()', ->
    # Fails: `quitAndInstall` will cause the app to quit and then it will not reopen.
    xit 'should emit an error if called before \'update-downloaded\' is emitted', (done) ->
      autoUpdater.on 'error', ->
        done()
      autoUpdater.quitAndInstall()

    it 'should not quit if all windows do not close', ->
      remote.getCurrentWindow().webContents.executeJavaScript 'window.onbeforeunload = function() { return false; }'
      autoUpdater.quitAndInstall()
