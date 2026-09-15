import type * as webViewInitModule from '@electron/internal/renderer/web-view/web-view-init';

const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

const webviewTag = mainFrame.getWebPreference('webviewTag');
const isWebView = mainFrame.getWebPreference('isWebView');

// Creates ipcRenderer up front so that messages from the browser have
// somewhere to go before anything imports it.
process._linkedBinding('electron_renderer_ipc');

// Load webview tag implementation.
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init') as typeof webViewInitModule;
  webViewInit(webviewTag, isWebView);
}
