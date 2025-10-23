// When using context isolation, the WebViewElement and the custom element
// methods have to be defined in the main world to be able to be registered.
//
// Note: The hidden values can only be read/set inside the same context, all
// methods that access the "internal" hidden value must be put in this file.
//
// Note: This file could be loaded in the main world of contextIsolation page,
// which runs in browserify environment instead of Node environment, all native
// modules must be passed from outside, all included files must be plain JS.

import type { SrcAttribute } from '@electron/internal/renderer/web-view/web-view-attributes';
import { WEB_VIEW_ATTRIBUTES, WEB_VIEW_ERROR_MESSAGES } from '@electron/internal/renderer/web-view/web-view-constants';
import { WebViewImpl, WebViewImplHooks, setupMethods } from '@electron/internal/renderer/web-view/web-view-impl';

const internals = new WeakMap<HTMLElement, WebViewImpl>();

// Return a WebViewElement class that is defined in this context.
const defineWebViewElement = (hooks: WebViewImplHooks) => {
  return class WebViewElement extends HTMLElement {
    static get observedAttributes () {
      return [
        WEB_VIEW_ATTRIBUTES.PARTITION,
        WEB_VIEW_ATTRIBUTES.SRC,
        WEB_VIEW_ATTRIBUTES.HTTPREFERRER,
        WEB_VIEW_ATTRIBUTES.USERAGENT,
        WEB_VIEW_ATTRIBUTES.NODEINTEGRATION,
        WEB_VIEW_ATTRIBUTES.NODEINTEGRATIONINSUBFRAMES,
        WEB_VIEW_ATTRIBUTES.PLUGINS,
        WEB_VIEW_ATTRIBUTES.DISABLEWEBSECURITY,
        WEB_VIEW_ATTRIBUTES.ALLOWPOPUPS,
        WEB_VIEW_ATTRIBUTES.PRELOAD,
        WEB_VIEW_ATTRIBUTES.BLINKFEATURES,
        WEB_VIEW_ATTRIBUTES.DISABLEBLINKFEATURES,
        WEB_VIEW_ATTRIBUTES.WEBPREFERENCES
      ];
    }

    constructor () {
      super();
      internals.set(this, new WebViewImpl(this, hooks));
    }

    getWebContentsId () {
      const internal = internals.get(this);
      if (!internal || !internal.guestInstanceId) {
        throw new Error(WEB_VIEW_ERROR_MESSAGES.NOT_ATTACHED);
      }
      return internal.guestInstanceId;
    }

    connectedCallback () {
      const internal = internals.get(this);
      if (!internal) {
        return;
      }
      if (!internal.elementAttached) {
        hooks.guestViewInternal.registerEvents(internal.viewInstanceId, {
          dispatchEvent: internal.dispatchEvent.bind(internal)
        });
        internal.elementAttached = true;
        (internal.attributes.get(WEB_VIEW_ATTRIBUTES.SRC) as SrcAttribute).parse();
      }
    }

    attributeChangedCallback (name: string, oldValue: any, newValue: any) {
      const internal = internals.get(this);
      if (internal) {
        internal.handleWebviewAttributeMutation(name, oldValue, newValue);
      }
    }

    disconnectedCallback () {
      const internal = internals.get(this);
      if (!internal) {
        return;
      }
      hooks.guestViewInternal.deregisterEvents(internal.viewInstanceId);
      if (internal.guestInstanceId) {
        hooks.guestViewInternal.detachGuest(internal.guestInstanceId);
      }
      internal.elementAttached = false;
      internal.reset();
    }
  };
};

// Register <webview> custom element.
const registerWebViewElement = (hooks: WebViewImplHooks) => {
  const WebViewElement = defineWebViewElement(hooks) as unknown as typeof ElectronInternal.WebViewElement;

  setupMethods(WebViewElement, hooks);

  // The customElements.define has to be called in a special scope.
  hooks.allowGuestViewElementDefinition(window, () => {
    window.customElements.define('webview', WebViewElement);
    window.WebView = WebViewElement;

    // Delete the callbacks so developers cannot call them and produce unexpected
    // behavior.
    delete WebViewElement.prototype.connectedCallback;
    delete WebViewElement.prototype.disconnectedCallback;
    delete WebViewElement.prototype.attributeChangedCallback;

    // Now that |observedAttributes| has been retrieved, we can hide it from
    // user code as well.
    // TypeScript is concerned that we're deleting a read-only attribute
    delete (WebViewElement as any).observedAttributes;
  });
};

// Prepare to register the <webview> element.
export const setupWebView = (hooks: WebViewImplHooks) => {
  const useCapture = true;
  const listener = (event: Event) => {
    if (document.readyState === 'loading') {
      return;
    }

    registerWebViewElement(hooks);

    window.removeEventListener(event.type, listener, useCapture);
  };

  window.addEventListener('readystatechange', listener, useCapture);
};
