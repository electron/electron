'use strict'

/* global nodeProcess, isolatedWorld */

const atomBinding = require('@electron/internal/common/atom-binding-setup')(nodeProcess._linkedBinding, 'renderer')

const v8Util = atomBinding('v8_util')

const webViewImpl = v8Util.getHiddenValue(isolatedWorld, 'web-view-impl')

if (webViewImpl) {
  // Must setup the WebView element in main world.
  const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element')
  setupWebView(v8Util, webViewImpl)
}

const isolatedWorldArgs = v8Util.getHiddenValue(isolatedWorld, 'isolated-world-args')

if (isolatedWorldArgs) {
  const { ipcRenderer, guestInstanceId, isHiddenPage, openerId, usesNativeWindowOpen } = isolatedWorldArgs
  require('@electron/internal/renderer/window-setup')(ipcRenderer, guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen)
}
