{EventEmitter} = require 'events'
SquirrelUpdate = require './auto-updater/squirrel-update-win'
app            = require 'app'
url            = require 'url'

class AutoUpdater extends EventEmitter

  quitAndInstall: ->
    SquirrelUpdate.processStart ->
      app.quit()

  setFeedUrl: (updateUrl) ->
    # set feed URL only when it hasn't been set before
    unless @updateUrl
      @updateUrl = updateUrl

  checkForUpdates: ->
    throw new Error('Update URL is not set') unless @updateUrl

    @emit 'checking-for-update'

    unless SquirrelUpdate.supported()
      @emit 'update-not-available'
      return

    SquirrelUpdate.download (error, update) =>
      if error?
        @emit 'update-not-available'
        return

      unless update?
        @emit 'update-not-available'
        return

      @emit 'update-available'

      SquirrelUpdate.update @updateUrl, (error) =>
        if error?
          @emit 'update-not-available'
          return

        # info about the newly installed version and a function any of the event listeners can call to restart the application
        @emit 'update-downloaded', {}, update.releaseNotes, update.version, new Date(), @updateUrl, => @quitAndInstall()

module.exports = new AutoUpdater()
