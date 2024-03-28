// Import necessary types
import type { WebViewImplHooks } from '@electron/internal/renderer/web-view/web-view-impl';

// Assuming `isolatedApi` is provided elsewhere in the codebase.
declare const isolatedApi: WebViewImplHooks;

// Check if `guestViewInternal` exists to proceed with the WebView setup.
if (isolatedApi.guestViewInternal) {
  // Dynamically import the module only when needed.
  // This is more efficient for cases where `isolatedApi.guestViewInternal` might be false,
  // as it avoids unnecessary loading of the module.
  import('@electron/internal/renderer/web-view/web-view-element')
    .then(webViewElementModule => {
      // Extract the setupWebView function
      const { setupWebView } = webViewElementModule as typeof import('@electron/internal/renderer/web-view/web-view-element');
      // Call the setupWebView function
      setupWebView(isolatedApi);
    })
    .catch(error => {
      // It's a good practice to handle potential errors in promises.
      console.error('Failed to load the web-view element module:', error);
    });
}
