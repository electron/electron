'use strict'

if (process.platform === 'linux' && process.type === 'renderer') {
  // On Linux we could not access clipboard in renderer process.
  const { getRemote } = require('@electron/internal/renderer/remote')
  module.exports = getRemote('clipboard')
} else {
  const clipboard = process.atomBinding('clipboard')

  // Read/write to find pasteboard over IPC since only main process is notified
  // of changes
  if (process.platform === 'darwin' && process.type === 'renderer') {
    const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')
    const errorUtils = require('@electron/internal/common/error-utils')

    const invoke = function (command, ...args) {
      const [ error, result ] = ipcRenderer.sendSync(command, ...args)

      if (error) {
        throw errorUtils.deserialize(error)
      } else {
        return result
      }
    }

    clipboard.readFindText = (...args) => invoke('ELECTRON_BROWSER_CLIPBOARD_READ_FIND_TEXT', ...args)
    clipboard.writeFindText = (...args) => invoke('ELECTRON_BROWSER_CLIPBOARD_WRITE_FIND_TEXT', ...args)
  }

  module.exports = clipboard
}
