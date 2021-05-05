import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as webViewElementModule from '@electron/internal/renderer/web-view/web-view-element';
import type * as guestViewInternalModule from '@electron/internal/renderer/web-view/guest-view-internal';

const contextBridge = process._linkedBinding('electron_renderer_context_bridge');
const v8Util = process._linkedBinding('electron_common_v8_util');
const { mainFrame: webFrame } = process._linkedBinding('electron_renderer_web_frame');

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
    const guestViewInternal = require('@electron/internal/renderer/web-view/guest-view-internal') as typeof guestViewInternalModule;
    if (contextIsolation) {
      contextBridge.exposeAPIInMainWorld('__electronGuestViewInternal__', guestViewInternal, true);
    } else {
      const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElementModule;
      setupWebView({
        guestViewInternal,
        allowGuestViewElementDefinition: webFrame.allowGuestViewElementDefinition,
        getWebFrameId: webFrame.getWebFrameId,
        setIsWebView: iframe => v8Util.setHiddenValue(iframe, 'isWebView', true)
      });
    }
  }

  if (guestInstanceId) {
    // Report focus/blur events of webview to browser.
    handleFocusBlur();
  }
}
