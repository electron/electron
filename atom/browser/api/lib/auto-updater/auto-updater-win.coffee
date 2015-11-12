app = require 'app'
url = require 'url'
{EventEmitter} = require 'events'

squirrelUpdate = require './squirrel-update-win'

class AutoUpdater extends EventEmitter
  quitAndInstall: ->
    squirrelUpdate.processStart()
    app.quit()

  setFeedUrl: (updateUrl) ->
    @updateUrl = updateUrl

  checkForUpdates: ->
    return @emitError 'Update URL is not set' unless @updateUrl
    return @emitError 'Can not find Squirrel' unless squirrelUpdate.supported()

    @emit 'checking-for-update'

    squirrelUpdate.download @updateUrl, (error, update) =>
      return @emitError error if error?
      return @emit 'update-not-available' unless update?

      @emit 'update-available'

      squirrelUpdate.update @updateUrl, (error) =>
        return @emitError error if error?

        {releaseNotes, version} = update
        # Following information is not available on Windows, so fake them.
        date = new Date
        url = @updateUrl

        @emit 'update-downloaded', {}, releaseNotes, version, date, url, => @quitAndInstall()

  # Private: Emit both error object and message, this is to keep compatibility
  # with Old APIs.
  emitError: (message) ->
    @emit 'error', new Error(message), message

module.exports = new AutoUpdater
