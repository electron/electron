'use strict'

const features = process.electronBinding('features')
const v8Util = process.electronBinding('v8_util')

const { enableRemoteModule } = v8Util.getHiddenValue(global, 'webPreferences')

// Renderer side modules, please sort alphabetically.
// A module is `enabled` if there is no explicit condition defined.
module.exports = [
  { name: 'crashReporter', loader: () => require('./crash-reporter') },
  { name: 'ipcRenderer', loader: () => require('./ipc-renderer') },
  { name: 'webFrame', loader: () => require('./web-frame') }
]

if (features.isDesktopCapturerEnabled()) {
  module.exports.push({ name: 'desktopCapturer', loader: () => require('./desktop-capturer') })
}

if (enableRemoteModule) {
  module.exports.push({ name: 'remote', loader: () => require('./remote') })
}
