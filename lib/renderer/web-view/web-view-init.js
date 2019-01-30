'use strict'

const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')
const v8Util = process.atomBinding('v8_util')

function handleFocusBlur (guestInstanceId) {
  // Note that while Chromium content APIs have observer for focus/blur, they
  // unfortunately do not work for webview.

  window.addEventListener('focus', () => {
    ipcRenderer.send('ELECTRON_GUEST_VIEW_MANAGER_FOCUS_CHANGE', true, guestInstanceId)
  })

  window.addEventListener('blur', () => {
    ipcRenderer.send('ELECTRON_GUEST_VIEW_MANAGER_FOCUS_CHANGE', false, guestInstanceId)
  })
}

module.exports = function (contextIsolation, webviewTag, guestInstanceId) {
  // Don't allow recursive `<webview>`.
  if (webviewTag && guestInstanceId == null) {
    const webViewImpl = require('@electron/internal/renderer/web-view/web-view-impl')
    if (contextIsolation) {
      v8Util.setHiddenValue(window, 'web-view-impl', webViewImpl)
    } else {
      const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element')
      setupWebView(v8Util, webViewImpl)
    }
  }

  if (guestInstanceId) {
    // Report focus/blur events of webview to browser.
    handleFocusBlur(guestInstanceId)
  }
}
