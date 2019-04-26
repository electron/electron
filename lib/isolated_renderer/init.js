'use strict'

/* global nodeProcess, isolatedWorld */

process.electronBinding = require('@electron/internal/common/atom-binding-setup').electronBindingSetup(nodeProcess._linkedBinding, 'renderer')

const v8Util = process.electronBinding('v8_util')

// The `lib/renderer/ipc-renderer-internal.js` module looks for the ipc object in the
// "ipc-internal" hidden value
v8Util.setHiddenValue(global, 'ipc-internal', v8Util.getHiddenValue(isolatedWorld, 'ipc-internal'))

const webViewImpl = v8Util.getHiddenValue(isolatedWorld, 'web-view-impl')

if (webViewImpl) {
  // Must setup the WebView element in main world.
  const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element')
  setupWebView(v8Util, webViewImpl)
}

const isolatedWorldArgs = v8Util.getHiddenValue(isolatedWorld, 'isolated-world-args')

if (isolatedWorldArgs) {
  const { guestInstanceId, isHiddenPage, openerId, usesNativeWindowOpen } = isolatedWorldArgs
  const { windowSetup } = require('@electron/internal/renderer/window-setup')
  windowSetup(guestInstanceId, openerId, isHiddenPage, usesNativeWindowOpen)
}
