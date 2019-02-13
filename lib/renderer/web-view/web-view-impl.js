'use strict'

const { webFrame, deprecate } = require('electron')

const v8Util = process.atomBinding('v8_util')
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')
const guestViewInternal = require('@electron/internal/renderer/web-view/guest-view-internal')
const webViewConstants = require('@electron/internal/renderer/web-view/web-view-constants')
const errorUtils = require('@electron/internal/common/error-utils')
const {
  syncMethods,
  asyncCallbackMethods,
  asyncPromiseMethods
} = require('@electron/internal/common/web-view-methods')

// ID generator.
let nextId = 0

const getNextId = function () {
  return ++nextId
}

// Represents the internal state of the WebView node.
class WebViewImpl {
  constructor (webviewNode) {
    this.webviewNode = webviewNode
    this.elementAttached = false
    this.beforeFirstNavigation = true
    this.hasFocus = false

    // on* Event handlers.
    this.on = {}

    // Create internal iframe element.
    this.internalElement = this.createInternalElement()
    const shadowRoot = this.webviewNode.attachShadow({ mode: 'open' })
    shadowRoot.innerHTML = '<!DOCTYPE html><style type="text/css">:host { display: flex; }</style>'
    this.setupWebViewAttributes()
    this.viewInstanceId = getNextId()
    shadowRoot.appendChild(this.internalElement)

    // Provide access to contentWindow.
    Object.defineProperty(this.webviewNode, 'contentWindow', {
      get: () => {
        return this.internalElement.contentWindow
      },
      enumerable: true
    })
  }

  createInternalElement () {
    const iframeElement = document.createElement('iframe')
    iframeElement.style.flex = '1 1 auto'
    iframeElement.style.width = '100%'
    iframeElement.style.border = '0'
    v8Util.setHiddenValue(iframeElement, 'internal', this)
    return iframeElement
  }

  // Resets some state upon reattaching <webview> element to the DOM.
  reset () {
    // If guestInstanceId is defined then the <webview> has navigated and has
    // already picked up a partition ID. Thus, we need to reset the initialization
    // state. However, it may be the case that beforeFirstNavigation is false BUT
    // guestInstanceId has yet to be initialized. This means that we have not
    // heard back from createGuest yet. We will not reset the flag in this case so
    // that we don't end up allocating a second guest.
    if (this.guestInstanceId) {
      guestViewInternal.destroyGuest(this.guestInstanceId)
      this.guestInstanceId = void 0
    }

    this.beforeFirstNavigation = true
    this.attributes[webViewConstants.ATTRIBUTE_PARTITION].validPartitionId = true

    // Since attachment swaps a local frame for a remote frame, we need our
    // internal iframe element to be local again before we can reattach.
    const newFrame = this.createInternalElement()
    const oldFrame = this.internalElement
    this.internalElement = newFrame
    oldFrame.parentNode.replaceChild(newFrame, oldFrame)
  }

  // Sets the <webview>.request property.
  setRequestPropertyOnWebViewNode (request) {
    Object.defineProperty(this.webviewNode, 'request', {
      value: request,
      enumerable: true
    })
  }

  // This observer monitors mutations to attributes of the <webview> and
  // updates the BrowserPlugin properties accordingly. In turn, updating
  // a BrowserPlugin property will update the corresponding BrowserPlugin
  // attribute, if necessary. See BrowserPlugin::UpdateDOMAttribute for more
  // details.
  handleWebviewAttributeMutation (attributeName, oldValue, newValue) {
    if (!this.attributes[attributeName] || this.attributes[attributeName].ignoreMutation) {
      return
    }

    // Let the changed attribute handle its own mutation
    this.attributes[attributeName].handleMutation(oldValue, newValue)
  }

  onElementResize () {
    const resizeEvent = new Event('resize')
    resizeEvent.newWidth = this.webviewNode.clientWidth
    resizeEvent.newHeight = this.webviewNode.clientHeight
    this.dispatchEvent(resizeEvent)
  }

  createGuest () {
    return guestViewInternal.createGuest(this.buildParams(), (event, guestInstanceId) => {
      this.attachGuestInstance(guestInstanceId)
    })
  }

  createGuestSync () {
    this.beforeFirstNavigation = false
    this.attachGuestInstance(guestViewInternal.createGuestSync(this.buildParams()))
  }

  dispatchEvent (webViewEvent) {
    this.webviewNode.dispatchEvent(webViewEvent)
  }

  // Adds an 'on<event>' property on the webview, which can be used to set/unset
  // an event handler.
  setupEventProperty (eventName) {
    const propertyName = `on${eventName.toLowerCase()}`
    return Object.defineProperty(this.webviewNode, propertyName, {
      get: () => {
        return this.on[propertyName]
      },
      set: (value) => {
        if (this.on[propertyName]) {
          this.webviewNode.removeEventListener(eventName, this.on[propertyName])
        }
        this.on[propertyName] = value
        if (value) {
          return this.webviewNode.addEventListener(eventName, value)
        }
      },
      enumerable: true
    })
  }

  // Updates state upon loadcommit.
  onLoadCommit (webViewEvent) {
    const oldValue = this.webviewNode.getAttribute(webViewConstants.ATTRIBUTE_SRC)
    const newValue = webViewEvent.url
    if (webViewEvent.isMainFrame && (oldValue !== newValue)) {
      // Touching the src attribute triggers a navigation. To avoid
      // triggering a page reload on every guest-initiated navigation,
      // we do not handle this mutation.
      this.attributes[webViewConstants.ATTRIBUTE_SRC].setValueIgnoreMutation(newValue)
    }
  }

  // Emits focus/blur events.
  onFocusChange () {
    const hasFocus = document.activeElement === this.webviewNode
    if (hasFocus !== this.hasFocus) {
      this.hasFocus = hasFocus
      this.dispatchEvent(new Event(hasFocus ? 'focus' : 'blur'))
    }
  }

  onAttach (storagePartitionId) {
    return this.attributes[webViewConstants.ATTRIBUTE_PARTITION].setValue(storagePartitionId)
  }

  buildParams () {
    const params = {
      instanceId: this.viewInstanceId,
      userAgentOverride: this.userAgentOverride
    }
    for (const attributeName in this.attributes) {
      if (this.attributes.hasOwnProperty(attributeName)) {
        params[attributeName] = this.attributes[attributeName].getValue()
      }
    }
    return params
  }

  attachGuestInstance (guestInstanceId) {
    if (!this.elementAttached) {
      // The element could be detached before we got response from browser.
      return
    }
    this.internalInstanceId = getNextId()
    this.guestInstanceId = guestInstanceId
    guestViewInternal.attachGuest(this.internalInstanceId, this.guestInstanceId, this.buildParams(), this.internalElement.contentWindow)
    // ResizeObserver is a browser global not recognized by "standard".
    /* globals ResizeObserver */
    // TODO(zcbenz): Should we deprecate the "resize" event? Wait, it is not
    // even documented.
    this.resizeObserver = new ResizeObserver(this.onElementResize.bind(this)).observe(this.internalElement)
  }
}

const setupAttributes = () => {
  require('@electron/internal/renderer/web-view/web-view-attributes')
}

const setupMethods = (WebViewElement) => {
  // WebContents associated with this webview.
  WebViewElement.prototype.getWebContents = function () {
    const { getRemote } = require('@electron/internal/renderer/remote')
    const getGuestWebContents = getRemote('getGuestWebContents')
    const internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal.guestInstanceId) {
      internal.createGuestSync()
    }
    return getGuestWebContents(internal.guestInstanceId)
  }

  // Focusing the webview should move page focus to the underlying iframe.
  WebViewElement.prototype.focus = function () {
    this.contentWindow.focus()
  }

  const getGuestInstanceId = function (self) {
    const internal = v8Util.getHiddenValue(self, 'internal')
    if (!internal.guestInstanceId) {
      throw new Error('The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.')
    }
    return internal.guestInstanceId
  }

  // Forward proto.foo* method calls to WebViewImpl.foo*.
  const createBlockHandler = function (method) {
    return function (...args) {
      const [error, result] = ipcRendererInternal.sendSync('ELECTRON_GUEST_VIEW_MANAGER_SYNC_CALL', getGuestInstanceId(this), method, args)
      if (error) {
        throw errorUtils.deserialize(error)
      } else {
        return result
      }
    }
  }
  for (const method of syncMethods) {
    WebViewElement.prototype[method] = createBlockHandler(method)
  }

  const createNonBlockHandler = function (method) {
    return function (...args) {
      const callback = (typeof args[args.length - 1] === 'function') ? args.pop() : null
      const requestId = getNextId()
      ipcRendererInternal.once(`ELECTRON_GUEST_VIEW_MANAGER_ASYNC_CALL_RESPONSE_${requestId}`, function (event, error, result) {
        if (error == null) {
          if (callback) callback(result)
        } else {
          throw errorUtils.deserialize(error)
        }
      })
      ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_ASYNC_CALL', requestId, getGuestInstanceId(this), method, args, callback != null)
    }
  }

  for (const method of asyncCallbackMethods) {
    WebViewElement.prototype[method] = createNonBlockHandler(method)
  }

  const createPromiseHandler = function (method) {
    return function (...args) {
      return new Promise((resolve, reject) => {
        const callback = (typeof args[args.length - 1] === 'function') ? args.pop() : null
        const requestId = getNextId()

        ipcRendererInternal.once(`ELECTRON_GUEST_VIEW_MANAGER_ASYNC_CALL_RESPONSE_${requestId}`, function (event, error, result) {
          if (error == null) {
            if (callback) {
              callback(result)
            }
            resolve(result)
          } else {
            reject(errorUtils.deserialize(error))
          }
        })
        ipcRendererInternal.send('ELECTRON_GUEST_VIEW_MANAGER_ASYNC_CALL', requestId, getGuestInstanceId(this), method, args, callback != null)
      })
    }
  }

  for (const method of asyncPromiseMethods) {
    WebViewElement.prototype[method] = createPromiseHandler(method)
  }

  WebViewElement.prototype.printToPDF = deprecate.promisify(WebViewElement.prototype.printToPDF)
}

module.exports = { setupAttributes, setupMethods, guestViewInternal, webFrame, WebViewImpl }
