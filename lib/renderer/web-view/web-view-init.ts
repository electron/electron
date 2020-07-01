import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

const v8Util = process._linkedBinding('electron_common_v8_util');

function handleFocusBlur (guestInstanceId: number) {
  // Note that while Chromium content APIs have observer for focus/blur, they
  // unfortunately do not work for webview.

  window.addEventListener('focus', () => {
    ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_FOCUS_CHANGE', true, guestInstanceId);
  });

  window.addEventListener('blur', () => {
    ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_FOCUS_CHANGE', false, guestInstanceId);
  });
}

export function webViewInit (
  contextIsolation: boolean, webviewTag: ElectronInternal.WebViewElement, guestInstanceId: number
) {
  // Don't allow recursive `<webview>`.
  if (webviewTag && guestInstanceId == null) {
    const { webViewImplModule } = require('@electron/internal/renderer/web-view/web-view-impl');
    if (contextIsolation) {
      v8Util.setHiddenValue(window, 'web-view-impl', webViewImplModule);
    } else {
      const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element');
      setupWebView(v8Util, webViewImplModule);
    }
  }

  if (guestInstanceId) {
    // Report focus/blur events of webview to browser.
    handleFocusBlur(guestInstanceId);
  }
}
