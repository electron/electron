const deprecate = require('electron').deprecate
const autoUpdater = process.platform === 'win32' ? require('./auto-updater/auto-updater-win') : require('./auto-updater/auto-updater-native')

// Deprecated.
deprecate.rename(autoUpdater, 'setFeedUrl', 'setFeedURL')

module.exports = autoUpdater
