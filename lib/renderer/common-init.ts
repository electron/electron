import type * as webViewInitModule from '@electron/internal/renderer/web-view/web-view-init';

const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

const webviewTag = mainFrame.getWebPreference('webviewTag');
const isWebView = mainFrame.getWebPreference('isWebView');

require('@electron/internal/renderer/ipc-native-setup');

// Load webview tag implementation.
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init') as typeof webViewInitModule;
  webViewInit(webviewTag, isWebView);
}
