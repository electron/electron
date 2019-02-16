'use strict'

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')
const ipcMessages = require('@electron/internal/common/ipc-messages')
const v8Util = process.atomBinding('v8_util')

function handleFocusBlur (guestInstanceId) {
  // Note that while Chromium content APIs have observer for focus/blur, they
  // unfortunately do not work for webview.

  window.addEventListener('focus', () => {
    ipcRendererInternal.send(ipcMessages.guestViewManager.focusChange, true, guestInstanceId)
  })

  window.addEventListener('blur', () => {
    ipcRendererInternal.send(ipcMessages.guestViewManager.focusChange, false, guestInstanceId)
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
