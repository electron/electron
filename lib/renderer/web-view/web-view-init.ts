import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as webViewImpl from '@electron/internal/renderer/web-view/web-view-impl';
import type * as webViewElement from '@electron/internal/renderer/web-view/web-view-element';

const v8Util = process._linkedBinding('electron_common_v8_util');

function handleFocusBlur () {
  // Note that while Chromium content APIs have observer for focus/blur, they
  // unfortunately do not work for webview.

  window.addEventListener('focus', () => {
    ipcRendererInternal.send(IPC_MESSAGES.GUEST_VIEW_MANAGER_FOCUS_CHANGE, true);
  });

  window.addEventListener('blur', () => {
    ipcRendererInternal.send(IPC_MESSAGES.GUEST_VIEW_MANAGER_FOCUS_CHANGE, false);
  });
}

export function webViewInit (contextIsolation: boolean, webviewTag: boolean, guestInstanceId: number) {
  // Don't allow recursive `<webview>`.
  if (webviewTag && !guestInstanceId) {
    const webViewImplModule = require('@electron/internal/renderer/web-view/web-view-impl') as typeof webViewImpl;
    if (contextIsolation) {
      v8Util.setHiddenValue(window, 'web-view-impl', webViewImplModule);
    } else {
      const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElement;
      setupWebView(v8Util, webViewImplModule);
    }
  }

  if (guestInstanceId) {
    // Report focus/blur events of webview to browser.
    handleFocusBlur();
  }
}
