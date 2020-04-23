'use strict';

/* global nodeProcess, isolatedWorld */

process.electronBinding = require('@electron/internal/common/electron-binding-setup').electronBindingSetup(nodeProcess._linkedBinding, 'renderer');

const v8Util = process.electronBinding('v8_util');

const webViewImpl = v8Util.getHiddenValue(isolatedWorld, 'web-view-impl');

if (webViewImpl) {
  // Must setup the WebView element in main world.
  const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element');
  setupWebView(v8Util, webViewImpl);
}
