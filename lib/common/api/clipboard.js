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

    clipboard.readFindText = (...args) => ipcRendererUtils.invokeSync('ELECTRON_BROWSER_CLIPBOARD_READ_FIND_TEXT', ...args)
    clipboard.writeFindText = (...args) => ipcRendererUtils.invokeSync('ELECTRON_BROWSER_CLIPBOARD_WRITE_FIND_TEXT', ...args)
  }

  module.exports = clipboard
}
