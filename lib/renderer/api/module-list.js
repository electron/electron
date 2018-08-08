const features = process.atomBinding('features')

// Renderer side modules, please sort alphabetically.
// A module is `enabled` if there is no explicit condition defined.
module.exports = [
  {
    name: 'desktopCapturer',
    file: 'desktop-capturer',
    enabled: features.isDesktopCapturerEnabled()
  },
  {name: 'ipcRenderer', file: 'ipc-renderer', enabled: true},
  {name: 'remote', file: 'remote', enabled: true},
  {name: 'screen', file: 'screen', enabled: true},
  {name: 'webFrame', file: 'web-frame', enabled: true}
]
