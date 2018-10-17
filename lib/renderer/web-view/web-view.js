'use strict'

const {ipcRenderer, remote, webFrame} = require('electron')

const v8Util = process.atomBinding('v8_util')
const guestViewInternal = require('./guest-view-internal')
const webViewConstants = require('./web-view-constants')

// An unique ID that can represent current context.
const contextId = v8Util.getHiddenValue(global, 'contextId')

// ID generator.
let nextId = 0

const getNextId = function () {
  return ++nextId
}

// A list of removed attributes from 3.0.
const removedAttributes = [
  'autoresize',
  'disableguestresize',
  'guestinstance'
]

// Represents the internal state of the WebView node.
class WebViewImpl {
  constructor (webviewNode) {
    this.webviewNode = webviewNode
    v8Util.setHiddenValue(this.webviewNode, 'internal', this)
    this.elementAttached = false
    this.beforeFirstNavigation = true
    this.hasFocus = false

    // Check for removed attributes.
    for (const attributeName of removedAttributes) {
      if (this.webviewNode.hasAttribute(attributeName)) {
        this.reportRemovedAttribute(attributeName)
      }
    }

    // on* Event handlers.
    this.on = {}

    // Create internal iframe element.
    this.internalElement = this.createInternalElement()
    const shadowRoot = this.webviewNode.attachShadow({mode: 'open'})
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

    this.webContents = null
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
    if (removedAttributes.includes(attributeName)) {
      this.reportRemovedAttribute(attributeName)
      return
    }

    if (!this.attributes[attributeName] || this.attributes[attributeName].ignoreMutation) {
      return
    }

    // Let the changed attribute handle its own mutation
    this.attributes[attributeName].handleMutation(oldValue, newValue)
  }

  onElementResize () {
    const resizeEvent = new Event('resize', { bubbles: true })
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
    this.webContents = remote.getGuestWebContents(this.guestInstanceId)
    guestViewInternal.attachGuest(this.internalInstanceId, this.guestInstanceId, this.buildParams(), this.internalElement.contentWindow)
    // ResizeObserver is a browser global not recognized by "standard".
    /* globals ResizeObserver */
    // TODO(zcbenz): Should we deprecate the "resize" event? Wait, it is not
    // even documented.
    this.resizeObserver = new ResizeObserver(this.onElementResize.bind(this)).observe(this.internalElement)
  }

  // TODO(zcbenz): Remove the warning in 4.0.
  reportRemovedAttribute (attributeName) {
    console.error(`The "${attributeName}" attribute has been removed from the <webview> tag,`,
                  'see https://github.com/electron/electron/issues/14120 for more.')
  }
}

// Registers <webview> custom element.
const registerWebViewElement = function () {
  const proto = Object.create(HTMLObjectElement.prototype)
  proto.createdCallback = function () {
    return new WebViewImpl(this)
  }
  proto.attributeChangedCallback = function (name, oldValue, newValue) {
    const internal = v8Util.getHiddenValue(this, 'internal')
    if (internal) {
      internal.handleWebviewAttributeMutation(name, oldValue, newValue)
    }
  }
  proto.detachedCallback = function () {
    const internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal) {
      return
    }
    guestViewInternal.deregisterEvents(internal.viewInstanceId)
    internal.elementAttached = false
    this.internalInstanceId = 0
    internal.reset()
  }
  proto.attachedCallback = function () {
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

  // Public-facing API methods.
  const methods = [
    'getURL',
    'loadURL',
    'getTitle',
    'isLoading',
    'isLoadingMainFrame',
    'isWaitingForResponse',
    'stop',
    'reload',
    'reloadIgnoringCache',
    'canGoBack',
    'canGoForward',
    'canGoToOffset',
    'clearHistory',
    'goBack',
    'goForward',
    'goToIndex',
    'goToOffset',
    'isCrashed',
    'setUserAgent',
    'getUserAgent',
    'openDevTools',
    'closeDevTools',
    'isDevToolsOpened',
    'isDevToolsFocused',
    'inspectElement',
    'setAudioMuted',
    'isAudioMuted',
    'undo',
    'redo',
    'cut',
    'copy',
    'paste',
    'pasteAndMatchStyle',
    'delete',
    'selectAll',
    'unselect',
    'replace',
    'replaceMisspelling',
    'findInPage',
    'stopFindInPage',
    'getId',
    'downloadURL',
    'inspectServiceWorker',
    'print',
    'printToPDF',
    'showDefinitionForSelection',
    'capturePage',
    'setZoomFactor',
    'setZoomLevel',
    'getZoomLevel',
    'getZoomFactor'
  ]
  const nonblockMethods = [
    'insertCSS',
    'insertText',
    'send',
    'sendInputEvent',
    'setLayoutZoomLevelLimits',
    'setVisualZoomLevelLimits'
  ]

  // Forward proto.foo* method calls to WebViewImpl.foo*.
  const createBlockHandler = function (m) {
    return function (...args) {
      const internal = v8Util.getHiddenValue(this, 'internal')
      if (internal.webContents) {
        return internal.webContents[m](...args)
      } else {
        throw new Error(`Cannot call ${m} because the webContents is unavailable. The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.`)
      }
    }
  }
  for (const method of methods) {
    proto[method] = createBlockHandler(method)
  }

  const createNonBlockHandler = function (m) {
    return function (...args) {
      const internal = v8Util.getHiddenValue(this, 'internal')
      ipcRenderer.send('ELECTRON_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', contextId, null, internal.guestInstanceId, m, ...args)
    }
  }
  for (const method of nonblockMethods) {
    proto[method] = createNonBlockHandler(method)
  }

  proto.executeJavaScript = function (code, hasUserGesture, callback) {
    const internal = v8Util.getHiddenValue(this, 'internal')
    if (typeof hasUserGesture === 'function') {
      callback = hasUserGesture
      hasUserGesture = false
    }
    const requestId = getNextId()
    ipcRenderer.send('ELECTRON_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', contextId, requestId, internal.guestInstanceId, 'executeJavaScript', code, hasUserGesture)
    ipcRenderer.once(`ELECTRON_RENDERER_ASYNC_CALL_TO_GUEST_VIEW_RESPONSE_${requestId}`, function (event, result) {
      if (callback) callback(result)
    })
  }

  // WebContents associated with this webview.
  proto.getWebContents = function () {
    const internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal.webContents) {
      internal.createGuestSync()
    }
    return internal.webContents
  }

  // Focusing the webview should move page focus to the underlying iframe.
  proto.focus = function () {
    this.contentWindow.focus()
  }

  window.WebView = webFrame.registerEmbedderCustomElement('webview', {
    prototype: proto
  })

  // Delete the callbacks so developers cannot call them and produce unexpected
  // behavior.
  delete proto.createdCallback
  delete proto.attachedCallback
  delete proto.detachedCallback
  delete proto.attributeChangedCallback
}

const useCapture = true

const listener = function (event) {
  if (document.readyState === 'loading') {
    return
  }
  registerWebViewElement()
  window.removeEventListener(event.type, listener, useCapture)
}

window.addEventListener('readystatechange', listener, true)

module.exports = WebViewImpl
