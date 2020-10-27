if (process.platform === 'win32') {
  module.exports = require('./auto-updater/auto-updater-win');
} else {
  module.exports = require('./auto-updater/auto-updater-native');
}
