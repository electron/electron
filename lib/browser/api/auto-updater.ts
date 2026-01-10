if (process.platform === 'win32') {
  if (process.windowsStore) {
    module.exports = require('./auto-updater/auto-updater-msix');
  } else {
    module.exports = require('./auto-updater/auto-updater-win');
  }
} else {
  module.exports = require('./auto-updater/auto-updater-native');
}
