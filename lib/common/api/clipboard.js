if (process.platform === 'linux' && process.type === 'renderer') {
  // On Linux we could not access clipboard in renderer process.
  module.exports = require('electron').remote.clipboard
} else {
  const {deprecate} = require('electron')
  const clipboard = process.atomBinding('clipboard')

  clipboard.readHtml = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.readHtml', 'clipboard.readHTML')
    }
    return clipboard.readHTML
  }

  clipboard.writeHtml = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.writeHtml', 'clipboard.writeHTML')
    }
    return clipboard.writeHTML
  }

  clipboard.readRtf = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.readRtf', 'clipboard.writeRTF')
    }
    return clipboard.readRTF
  }

  clipboard.writeRtf = function () {
    if (!process.noDeprecations) {
      deprecate.warn('clipboard.writeRtf', 'clipboard.writeRTF')
    }
    return clipboard.writeRTF
  }

  // Read/write to find pasteboard over IPC since only main process is notified
  // of changes
  if (process.platform === 'darwin' && process.type === 'renderer') {
    clipboard.readFindText = require('electron').remote.clipboard.readFindText
    clipboard.writeFindText = require('electron').remote.clipboard.writeFindText
  }

  module.exports = clipboard
}
