'use strict'

const features = process.atomBinding('features')
const v8Util = process.atomBinding('v8_util')

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
  { name: 'screen', file: 'screen' },
  { name: 'webFrame', file: 'web-frame' }
]
