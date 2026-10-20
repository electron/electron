import type * as webViewElementModule from '@electron/internal/renderer/web-view/web-view-element';
import type { WebViewImplHooks } from '@electron/internal/renderer/web-view/web-view-impl';

declare const isolatedApi: WebViewImplHooks & Partial<import('../renderer/web-view/webview2-element').WebView2Hooks>;

if (isolatedApi.webview2Internal) {
  const { setupWebView2 } =
    require('@electron/internal/renderer/web-view/webview2-element') as typeof import('../renderer/web-view/webview2-element');
  setupWebView2({ ...isolatedApi, webview2Internal: isolatedApi.webview2Internal });
}

if (isolatedApi.guestViewInternal) {
  // Must setup the WebView element in main world.
  const { setupWebView } =
    require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElementModule;
  setupWebView(isolatedApi);
}
