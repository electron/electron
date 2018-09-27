'use strict'

const features = process.atomBinding('features')

module.exports = [
  {
    name: 'crashReporter',
    load: () => require('@electron/internal/common/api/crash-reporter')
  },
  {
    name: 'desktopCapturer',
    load: () => require('@electron/internal/renderer/api/desktop-capturer'),
    enabled: features.isDesktopCapturerEnabled()
  },
  {
    name: 'ipcRenderer',
    load: () => require('@electron/internal/sandboxed_renderer/api/ipc-renderer')
  },
  {
    name: 'isPromise',
    load: () => require('@electron/internal/common/api/is-promise'),
    private: true
  },
  {
    name: 'nativeImage',
    load: () => require('@electron/internal/common/api/native-image')
  },
  {
    name: 'remote',
    load: () => require('@electron/internal/renderer/api/remote')
  },
  {
    name: 'webFrame',
    load: () => require('@electron/internal/renderer/api/web-frame')
  }
]
