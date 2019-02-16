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
    const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils')
    const ipcMessages = require('@electron/internal/common/ipc-messages')

    clipboard.readFindText = (...args) => ipcRendererUtils.invokeSync(ipcMessages.clipboard.readFindText, ...args)
    clipboard.writeFindText = (...args) => ipcRendererUtils.invokeSync(ipcMessages.clipboard.writeFindText, ...args)
  }

  module.exports = clipboard
}
