/* global isolatedApi */

import type * as webViewElementModule from '@electron/internal/renderer/web-view/web-view-element';
import type * as guestViewInternalModule from '@electron/internal/renderer/web-view/guest-view-internal';

const guestViewInternal = (window as any).__electronGuestViewInternal__ as typeof guestViewInternalModule;
delete (window as any).__electronGuestViewInternal__;

if (guestViewInternal) {
  // Must setup the WebView element in main world.
  const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElementModule;
  setupWebView({ guestViewInternal, ...isolatedApi });
}
