import type * as guestViewInternalModule from '@electron/internal/renderer/web-view/guest-view-internal';
import type * as webViewElementModule from '@electron/internal/renderer/web-view/web-view-element';

const v8Util = process._linkedBinding('electron_common_v8_util');
const { mainFrame: webFrame } = process._linkedBinding('electron_renderer_web_frame');

export function webViewInit(webviewTag: boolean, isWebView: boolean, webview2Tag = false) {
  if (webview2Tag && !isWebView) {
    const webview2Internal =
      require('@electron/internal/renderer/web-view/webview2-internal') as typeof import('./webview2-internal');
    if (process.contextIsolated) {
      v8Util.setHiddenValue(window, 'webview2Internal', webview2Internal);
    } else {
      const { setupWebView2 } =
        require('@electron/internal/renderer/web-view/webview2-element') as typeof import('./webview2-element');
      setupWebView2({
        webview2Internal,
        allowGuestViewElementDefinition: webFrame.allowGuestViewElementDefinition,
        setIsWebView: (iframe) => v8Util.setHiddenValue(iframe, 'isWebView', true)
      });
    }
  }
  // Don't allow recursive `<webview>`.
  if (webviewTag && !isWebView) {
    const guestViewInternal =
      require('@electron/internal/renderer/web-view/guest-view-internal') as typeof guestViewInternalModule;
    if (process.contextIsolated) {
      v8Util.setHiddenValue(window, 'guestViewInternal', guestViewInternal);
    } else {
      const { setupWebView } =
        require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElementModule;
      setupWebView({
        guestViewInternal,
        allowGuestViewElementDefinition: webFrame.allowGuestViewElementDefinition,
        setIsWebView: (iframe) => v8Util.setHiddenValue(iframe, 'isWebView', true)
      });
    }
  }
}
