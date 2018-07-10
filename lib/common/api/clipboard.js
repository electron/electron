if (process.platform === 'linux' && process.type === 'renderer') {
  // On Linux we could not access clipboard in renderer process.
  module.exports = require('electron').remote.clipboard
} else {
  const clipboard = process.atomBinding('clipboard')

  // Read/write to find pasteboard over IPC since only main process is notified
  // of changes
  if (process.platform === 'darwin' && process.type === 'renderer') {
    clipboard.readFindText = require('electron').remote.clipboard.readFindText
    clipboard.writeFindText = require('electron').remote.clipboard.writeFindText
  }

  module.exports = clipboard
}
