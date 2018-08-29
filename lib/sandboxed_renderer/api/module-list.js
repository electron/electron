const features = process.atomBinding('features')

module.exports = [
  {
    name: 'CallbacksRegistry',
    load: () => require('../../common/api/callbacks-registry'),
    private: true
  },
  {
    name: 'crashReporter',
    load: () => require('../../common/api/crash-reporter')
  },
  {
    name: 'desktopCapturer',
    load: () => require('../../renderer/api/desktop-capturer'),
    enabled: features.isDesktopCapturerEnabled()
  },
  {
    name: 'ipcRenderer',
    load: () => require('./ipc-renderer')
  },
  {
    name: 'isPromise',
    load: () => require('../../common/api/is-promise'),
    private: true
  },
  {
    name: 'nativeImage',
    load: () => require('../../common/api/native-image')
  },
  {
    name: 'remote',
    load: () => require('../../renderer/api/remote')
  },
  {
    name: 'webFrame',
    load: () => require('../../renderer/api/web-frame')
  }
]
