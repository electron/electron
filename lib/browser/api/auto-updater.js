const autoUpdater = process.platform === 'win32' ? require('./auto-updater/auto-updater-win') : require('./auto-updater/auto-updater-native')
module.exports = autoUpdater
