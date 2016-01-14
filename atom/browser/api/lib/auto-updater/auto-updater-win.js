var AutoUpdater, EventEmitter, app, squirrelUpdate, url,
  extend = function(child, parent) { for (var key in parent) { if (hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; },
  hasProp = {}.hasOwnProperty;

app = require('electron').app;

EventEmitter = require('events').EventEmitter;

url = require('url');

squirrelUpdate = require('./squirrel-update-win');

AutoUpdater = (function(superClass) {
  extend(AutoUpdater, superClass);

  function AutoUpdater() {
    return AutoUpdater.__super__.constructor.apply(this, arguments);
  }

  AutoUpdater.prototype.quitAndInstall = function() {
    squirrelUpdate.processStart();
    return app.quit();
  };

  AutoUpdater.prototype.setFeedURL = function(updateURL) {
    return this.updateURL = updateURL;
  };

  AutoUpdater.prototype.checkForUpdates = function() {
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
  };


  // Private: Emit both error object and message, this is to keep compatibility
  // with Old APIs.
  AutoUpdater.prototype.emitError = function(message) {
    return this.emit('error', new Error(message), message);
  };

  return AutoUpdater;

})(EventEmitter);

module.exports = new AutoUpdater;
