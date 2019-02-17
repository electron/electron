let platformDependentAPI: Electron.AutoUpdater

if (process.platform === 'win32') {
  platformDependentAPI = require('@electron/internal/browser/api/auto-updater/auto-updater-win').default
} else {
  platformDependentAPI = require('@electron/internal/browser/api/auto-updater/auto-updater-native').default
}

export default platformDependentAPI
