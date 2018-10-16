'use strict'

if (process.platform === 'win32') {
  module.exports = require('@electron/internal/browser/api/auto-updater/auto-updater-win')
} else {
  module.exports = require('@electron/internal/browser/api/auto-updater/auto-updater-native')
}
