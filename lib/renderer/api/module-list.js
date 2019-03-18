'use strict'

const features = process.electronBinding('features')
const v8Util = process.electronBinding('v8_util')

const enableRemoteModule = v8Util.getHiddenValue(global, 'enableRemoteModule')

// Renderer side modules, please sort alphabetically.
// A module is `enabled` if there is no explicit condition defined.
module.exports = [
  { name: 'crashReporter', file: 'crash-reporter', enabled: true },
  {
    name: 'desktopCapturer',
    file: 'desktop-capturer',
    enabled: features.isDesktopCapturerEnabled()
  },
  { name: 'ipcRenderer', file: 'ipc-renderer' },
  { name: 'remote', file: 'remote', enabled: enableRemoteModule },
  { name: 'webFrame', file: 'web-frame' }
]
