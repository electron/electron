if (process.platform === 'linux' && process.type === 'renderer') {
  // On Linux we could not access clipboard in renderer process.
  module.exports = require('electron').remote.clipboard
} else {
  const {deprecate} = require('electron')
  const clipboard = process.atomBinding('clipboard')

  // TODO(codebytere): remove in 3.0
  clipboard.readHtml = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.readHtml', 'clipboard.readHTML')
    }
    return clipboard.readHTML()
  }

  // TODO(codebytere): remove in 3.0
  clipboard.writeHtml = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.writeHtml', 'clipboard.writeHTML')
    }
    return clipboard.writeHTML()
  }

  // TODO(codebytere): remove in 3.0
  clipboard.readRtf = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.readRtf', 'clipboard.writeRTF')
    }
    return clipboard.readRTF()
  }

  // TODO(codebytere): remove in 3.0
  clipboard.writeRtf = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.writeRtf', 'clipboard.writeRTF')
    }
    return clipboard.writeRTF()
  }

  // Read/write to find pasteboard over IPC since only main process is notified
  // of changes
  if (process.platform === 'darwin' && process.type === 'renderer') {
    clipboard.readFindText = require('electron').remote.clipboard.readFindText
    clipboard.writeFindText = require('electron').remote.clipboard.writeFindText
  }

  module.exports = clipboard
}
