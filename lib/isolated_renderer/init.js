'use strict'

/* global nodeProcess, isolatedWorld */

// Note: Don't use "process", as it will be replaced by browserify's one.
const v8Util = nodeProcess.atomBinding('v8_util')

const isolatedWorldArgs = v8Util.getHiddenValue(isolatedWorld, 'isolated-world-args')
const { webViewImpl, ipcRenderer, guestInstanceId, hiddenPage, openerId, usesNativeWindowOpen } = isolatedWorldArgs

if (webViewImpl) {
  // Must setup the WebView element in main world.
  const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element')
  setupWebView(v8Util, webViewImpl)
}

require('@electron/internal/renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage, usesNativeWindowOpen)
