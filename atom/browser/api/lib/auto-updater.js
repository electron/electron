var autoUpdater, deprecate;

deprecate = require('electron').deprecate;

autoUpdater = process.platform === 'win32' ? require('./auto-updater/auto-updater-win') : require('./auto-updater/auto-updater-native');

// Deprecated.
deprecate.rename(autoUpdater, 'setFeedUrl', 'setFeedURL');

module.exports = autoUpdater;
