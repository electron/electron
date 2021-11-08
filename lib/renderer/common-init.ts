import { ipcRenderer } from 'electron';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

import type * as webViewInitModule from '@electron/internal/renderer/web-view/web-view-init';
import type * as windowSetupModule from '@electron/internal/renderer/window-setup';
import type * as webFrameInitModule from '@electron/internal/renderer/web-frame-init';
import type * as securityWarningsModule from '@electron/internal/renderer/security-warnings';

const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');
const v8Util = process._linkedBinding('electron_common_v8_util');

const nodeIntegration = mainFrame.getWebPreference('nodeIntegration');
const webviewTag = mainFrame.getWebPreference('webviewTag');
const isHiddenPage = mainFrame.getWebPreference('hiddenPage');
const nativeWindowOpen = mainFrame.getWebPreference('nativeWindowOpen') || process.sandboxed;
const isWebView = mainFrame.getWebPreference('isWebView');
const openerId = mainFrame.getWebPreference('openerId');

// ElectronApiServiceImpl will look for the "ipcNative" hidden object when
// invoking the 'onMessage' callback.
v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal: boolean, channel: string, ports: MessagePort[], args: any[], senderId: number) {
    if (internal && senderId !== 0) {
      console.error(`Message ${channel} sent by unexpected WebContents (${senderId})`);
      return;
    }
    const sender = internal ? ipcRendererInternal : ipcRenderer;
    sender.emit(channel, { sender, senderId, ports }, ...args);
  }
});

switch (window.location.protocol) {
  case 'devtools:': {
    // Override some inspector APIs.
    require('@electron/internal/renderer/inspector');
    break;
  }
  case 'chrome-extension:': {
    break;
  }
  case 'chrome:': {
    break;
  }
  default: {
    // Override default web functions.
    const { windowSetup } = require('@electron/internal/renderer/window-setup') as typeof windowSetupModule;
    windowSetup(isWebView, openerId, isHiddenPage, nativeWindowOpen);
  }
}

// Load webview tag implementation.
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init') as typeof webViewInitModule;
  webViewInit(webviewTag, isWebView);
}

const { webFrameInit } = require('@electron/internal/renderer/web-frame-init') as typeof webFrameInitModule;
webFrameInit();

// Warn about security issues
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings') as typeof securityWarningsModule;
  securityWarnings(nodeIntegration);
}
