'use strict'

const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')

const { guestInstanceId, openerId } = process
const hiddenPage = process.argv.includes('--hidden-page')
const usesNativeWindowOpen = process.argv.includes('--native-window-open')

require('@electron/internal/renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage, usesNativeWindowOpen)
