'use strict'

// When using context isolation, the WebViewElement and the custom element
// methods have to be defined in the main world to be able to be registered.
//
// Note: The hidden values can only be read/set inside the same context, all
// methods that access the "internal" hidden value must be put in this file.
//
// Note: This file could be loaded in the main world of contextIsolation page,
// which runs in browserify environment instead of Node environment, all native
// modules must be passed from outside, all included files must be plain JS.

const webViewConstants = require('@electron/internal/renderer/web-view/web-view-constants')

// Return a WebViewElement class that is defined in this context.
const defineWebViewElement = (v8Util, webViewImpl) => {
  const { guestViewInternal, WebViewImpl } = webViewImpl
  return class WebViewElement extends HTMLElement {
    static get observedAttributes () {
      return [
        webViewConstants.ATTRIBUTE_PARTITION,
        webViewConstants.ATTRIBUTE_SRC,
        webViewConstants.ATTRIBUTE_HTTPREFERRER,
        webViewConstants.ATTRIBUTE_USERAGENT,
        webViewConstants.ATTRIBUTE_NODEINTEGRATION,
        webViewConstants.ATTRIBUTE_NODEINTEGRATIONINSUBFRAMES,
        webViewConstants.ATTRIBUTE_PLUGINS,
        webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY,
        webViewConstants.ATTRIBUTE_ALLOWPOPUPS,
        webViewConstants.ATTRIBUTE_ENABLEREMOTEMODULE,
        webViewConstants.ATTRIBUTE_PRELOAD,
        webViewConstants.ATTRIBUTE_BLINKFEATURES,
        webViewConstants.ATTRIBUTE_DISABLEBLINKFEATURES,
        webViewConstants.ATTRIBUTE_WEBPREFERENCES
      ]
    }

    constructor () {
      super()
      v8Util.setHiddenValue(this, 'internal', new WebViewImpl(this))
    }

    connectedCallback () {
      const internal = v8Util.getHiddenValue(this, 'internal')
      if (!internal) {
        return
      }
      if (!internal.elementAttached) {
        guestViewInternal.registerEvents(internal, internal.viewInstanceId)
        internal.elementAttached = true
        internal.attributes[webViewConstants.ATTRIBUTE_SRC].parse()
      }
    }

    attributeChangedCallback (name, oldValue, newValue) {
      const internal = v8Util.getHiddenValue(this, 'internal')
      if (internal) {
        internal.handleWebviewAttributeMutation(name, oldValue, newValue)
      }
    }

    disconnectedCallback () {
      const internal = v8Util.getHiddenValue(this, 'internal')
      if (!internal) {
        return
      }
      guestViewInternal.deregisterEvents(internal.viewInstanceId)
      internal.elementAttached = false
      this.internalInstanceId = 0
      internal.reset()
    }
  }
}

// Register <webview> custom element.
const registerWebViewElement = (v8Util, webViewImpl) => {
  const WebViewElement = defineWebViewElement(v8Util, webViewImpl)

  webViewImpl.setupMethods(WebViewElement)

  // The customElements.define has to be called in a special scope.
  webViewImpl.webFrame.allowGuestViewElementDefinition(window, () => {
    window.customElements.define('webview', WebViewElement)
    window.WebView = WebViewElement

    // Delete the callbacks so developers cannot call them and produce unexpected
    // behavior.
    delete WebViewElement.prototype.connectedCallback
    delete WebViewElement.prototype.disconnectedCallback
    delete WebViewElement.prototype.attributeChangedCallback

    // Now that |observedAttributes| has been retrieved, we can hide it from
    // user code as well.
    delete WebViewElement.observedAttributes
  })
}

// Prepare to register the <webview> element.
const setupWebView = (v8Util, webViewImpl) => {
  const useCapture = true
  window.addEventListener('readystatechange', function listener (event) {
    if (document.readyState === 'loading') {
      return
    }
    webViewImpl.setupAttributes()
    registerWebViewElement(v8Util, webViewImpl)
    window.removeEventListener(event.type, listener, useCapture)
  }, useCapture)
}

module.exports = { setupWebView }
