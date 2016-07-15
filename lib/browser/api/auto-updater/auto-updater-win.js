'use strict'

const app = require('electron').app
const EventEmitter = require('events').EventEmitter
const squirrelUpdate = require('./squirrel-update-win')
const util = require('util')

function AutoUpdater () {
  EventEmitter.call(this)
}

util.inherits(AutoUpdater, EventEmitter)

AutoUpdater.prototype.quitAndInstall = function () {
  if (!this.updateAvailable) {
    return this.emitError('No update available, can\'t quit and install')
  }
  squirrelUpdate.processStart()
  return app.quit()
}

AutoUpdater.prototype.getFeedURL = function () {
  return this.updateURL
}

AutoUpdater.prototype.setFeedURL = function (updateURL, headers) {
  this.updateURL = updateURL
}

AutoUpdater.prototype.checkForUpdates = function () {
  if (!this.updateURL) {
    return this.emitError('Update URL is not set')
  }
  if (!squirrelUpdate.supported()) {
    return this.emitError('Can not find Squirrel')
  }
  this.emit('checking-for-update')
  squirrelUpdate.download(this.updateURL, (error, update) => {
    if (error != null) {
      return this.emitError(error)
    }
    if (update == null) {
      this.updateAvailable = false
      return this.emit('update-not-available')
    }
    this.updateAvailable = true
    this.emit('update-available')
    squirrelUpdate.update(this.updateURL, (error) => {
      var date, releaseNotes, version
      if (error != null) {
        return this.emitError(error)
      }
      releaseNotes = update.releaseNotes
      version = update.version

      // Following information is not available on Windows, so fake them.
      date = new Date()
      this.emit('update-downloaded', {}, releaseNotes, version, date, this.updateURL, () => {
        this.quitAndInstall()
      })
    })
  })
}

// Private: Emit both error object and message, this is to keep compatibility
// with Old APIs.
AutoUpdater.prototype.emitError = function (message) {
  return this.emit('error', new Error(message), message)
}

module.exports = new AutoUpdater()
