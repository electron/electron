'use strict';

const app = require('electron').app;
const EventEmitter = require('events').EventEmitter;
const url = require('url');
const squirrelUpdate = require('./squirrel-update-win');

class AutoUpdater extends EventEmitter {
  constructor() {
    super();
  }

  quitAndInstall() {
    squirrelUpdate.processStart();
    return app.quit();
  }

  setFeedURL(updateURL) {
    return this.updateURL = updateURL;
  }

  checkForUpdates() {
    if (!this.updateURL) {
      return this.emitError('Update URL is not set');
    }
    if (!squirrelUpdate.supported()) {
      return this.emitError('Can not find Squirrel');
    }
    this.emit('checking-for-update');
    return squirrelUpdate.download(this.updateURL, (function(_this) {
      return function(error, update) {
        if (error != null) {
          return _this.emitError(error);
        }
        if (update == null) {
          return _this.emit('update-not-available');
        }
        _this.emit('update-available');
        return squirrelUpdate.update(_this.updateURL, function(error) {
          var date, releaseNotes, version;
          if (error != null) {
            return _this.emitError(error);
          }
          releaseNotes = update.releaseNotes, version = update.version;

          // Following information is not available on Windows, so fake them.
          date = new Date;
          url = _this.updateURL;
          return _this.emit('update-downloaded', {}, releaseNotes, version, date, url, function() {
            return _this.quitAndInstall();
          });
        });
      };
    })(this));
  }

  // Private: Emit both error object and message, this is to keep compatibility
  // with Old APIs.
  emitError(message) {
    return this.emit('error', new Error(message), message);
  }
}

module.exports = new AutoUpdater;
