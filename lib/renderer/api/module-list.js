const features = process.atomBinding('features')

// Renderer side modules, please sort alphabetically.
// A module is `enabled` if there is no explicit condition defined.
module.exports = [
  {
    name: 'desktopCapturer',
    file: 'desktop-capturer',
    enabled: features.isDesktopCapturerEnabled()
  },
  {name: 'ipcRenderer', file: 'ipc-renderer'},
  {name: 'remote', file: 'remote'},
  {name: 'screen', file: 'screen'},
  {name: 'webFrame', file: 'web-frame'}
]
