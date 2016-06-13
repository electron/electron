const EventEmitter = require('events').EventEmitter
const autoUpdater = process.atomBinding('auto_updater').autoUpdater

Object.setPrototypeOf(autoUpdater, EventEmitter.prototype)

autoUpdater.setFeedURL = function (url, headers) {
  return autoUpdater._setFeedURL(url, headers || {})
}

module.exports = autoUpdater
