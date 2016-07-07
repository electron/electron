'use strict'

const webFrame = require('electron').webFrame
const remote = require('electron').remote
const ipcRenderer = require('electron').ipcRenderer

const v8Util = process.atomBinding('v8_util')
const guestViewInternal = require('./guest-view-internal')
const webViewConstants = require('./web-view-constants')

var hasProp = {}.hasOwnProperty

// ID generator.
var nextId = 0

var getNextId = function () {
  return ++nextId
}

// Represents the internal state of the WebView node.
var WebViewImpl = (function () {
  function WebViewImpl (webviewNode) {
    var shadowRoot
    this.webviewNode = webviewNode
    v8Util.setHiddenValue(this.webviewNode, 'internal', this)
    this.attached = false
    this.elementAttached = false
    this.beforeFirstNavigation = true

    // on* Event handlers.
    this.on = {}
    this.browserPluginNode = this.createBrowserPluginNode()
    shadowRoot = this.webviewNode.createShadowRoot()
    shadowRoot.innerHTML = '<style>:host { display: flex; }</style>'
    this.setupWebViewAttributes()
    this.setupFocusPropagation()
    this.viewInstanceId = getNextId()
    shadowRoot.appendChild(this.browserPluginNode)

    // Subscribe to host's zoom level changes.
    this.onZoomLevelChanged = (zoomLevel) => {
      this.webviewNode.setZoomLevel(zoomLevel)
    }
    webFrame.on('zoom-level-changed', this.onZoomLevelChanged)

    this.onVisibilityChanged = (event, visibilityState) => {
      this.webviewNode.send('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', visibilityState)
    }
    ipcRenderer.on('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', this.onVisibilityChanged)
  }

  WebViewImpl.prototype.createBrowserPluginNode = function () {
    // We create BrowserPlugin as a custom element in order to observe changes
    // to attributes synchronously.
    var browserPluginNode = new WebViewImpl.BrowserPlugin()
    v8Util.setHiddenValue(browserPluginNode, 'internal', this)
    return browserPluginNode
  }

  // Resets some state upon reattaching <webview> element to the DOM.
  WebViewImpl.prototype.reset = function () {
    // Unlisten the zoom-level-changed event.
    webFrame.removeListener('zoom-level-changed', this.onZoomLevelChanged)
    ipcRenderer.removeListener('ELECTRON_RENDERER_WINDOW_VISIBILITY_CHANGE', this.onVisibilityChanged)

    // If guestInstanceId is defined then the <webview> has navigated and has
    // already picked up a partition ID. Thus, we need to reset the initialization
    // state. However, it may be the case that beforeFirstNavigation is false BUT
    // guestInstanceId has yet to be initialized. This means that we have not
    // heard back from createGuest yet. We will not reset the flag in this case so
    // that we don't end up allocating a second guest.
    if (this.guestInstanceId) {
      guestViewInternal.destroyGuest(this.guestInstanceId)
      this.webContents = null
      this.guestInstanceId = void 0
      this.beforeFirstNavigation = true
      this.attributes[webViewConstants.ATTRIBUTE_PARTITION].validPartitionId = true
    }
    this.internalInstanceId = 0
  }

  // Sets the <webview>.request property.
  WebViewImpl.prototype.setRequestPropertyOnWebViewNode = function (request) {
    return Object.defineProperty(this.webviewNode, 'request', {
      value: request,
      enumerable: true
    })
  }

  WebViewImpl.prototype.setupFocusPropagation = function () {
    if (!this.webviewNode.hasAttribute('tabIndex')) {
      // <webview> needs a tabIndex in order to be focusable.
      // TODO(fsamuel): It would be nice to avoid exposing a tabIndex attribute
      // to allow <webview> to be focusable.
      // See http://crbug.com/231664.
      this.webviewNode.setAttribute('tabIndex', -1)
    }

    // Focus the BrowserPlugin when the <webview> takes focus.
    this.webviewNode.addEventListener('focus', () => {
      this.browserPluginNode.focus()
    })

    // Blur the BrowserPlugin when the <webview> loses focus.
    this.webviewNode.addEventListener('blur', () => {
      this.browserPluginNode.blur()
    })
  }

  // This observer monitors mutations to attributes of the <webview> and
  // updates the BrowserPlugin properties accordingly. In turn, updating
  // a BrowserPlugin property will update the corresponding BrowserPlugin
  // attribute, if necessary. See BrowserPlugin::UpdateDOMAttribute for more
  // details.
  WebViewImpl.prototype.handleWebviewAttributeMutation = function (attributeName, oldValue, newValue) {
    if (!this.attributes[attributeName] || this.attributes[attributeName].ignoreMutation) {
      return
    }

    // Let the changed attribute handle its own mutation
    return this.attributes[attributeName].handleMutation(oldValue, newValue)
  }

  WebViewImpl.prototype.handleBrowserPluginAttributeMutation = function (attributeName, oldValue, newValue) {
    if (attributeName === webViewConstants.ATTRIBUTE_INTERNALINSTANCEID && !oldValue && !!newValue) {
      this.browserPluginNode.removeAttribute(webViewConstants.ATTRIBUTE_INTERNALINSTANCEID)
      this.internalInstanceId = parseInt(newValue)

      // Track when the element resizes using the element resize callback.
      webFrame.registerElementResizeCallback(this.internalInstanceId, this.onElementResize.bind(this))
      if (!this.guestInstanceId) {
        return
      }
      return guestViewInternal.attachGuest(this.internalInstanceId, this.guestInstanceId, this.buildParams())
    }
  }

  WebViewImpl.prototype.onSizeChanged = function (webViewEvent) {
    var maxHeight, maxWidth, minHeight, minWidth, newHeight, newWidth, node, width
    newWidth = webViewEvent.newWidth
    newHeight = webViewEvent.newHeight
    node = this.webviewNode
    width = node.offsetWidth

    // Check the current bounds to make sure we do not resize <webview>
    // outside of current constraints.
    maxWidth = this.attributes[webViewConstants.ATTRIBUTE_MAXWIDTH].getValue() | width
    maxHeight = this.attributes[webViewConstants.ATTRIBUTE_MAXHEIGHT].getValue() | width
    minWidth = this.attributes[webViewConstants.ATTRIBUTE_MINWIDTH].getValue() | width
    minHeight = this.attributes[webViewConstants.ATTRIBUTE_MINHEIGHT].getValue() | width
    minWidth = Math.min(minWidth, maxWidth)
    minHeight = Math.min(minHeight, maxHeight)
    if (!this.attributes[webViewConstants.ATTRIBUTE_AUTOSIZE].getValue() || (newWidth >= minWidth && newWidth <= maxWidth && newHeight >= minHeight && newHeight <= maxHeight)) {
      node.style.width = newWidth + 'px'
      node.style.height = newHeight + 'px'

      // Only fire the DOM event if the size of the <webview> has actually
      // changed.
      return this.dispatchEvent(webViewEvent)
    }
  }

  WebViewImpl.prototype.onElementResize = function (newSize) {
    // Dispatch the 'resize' event.
    var resizeEvent
    resizeEvent = new Event('resize', {
      bubbles: true
    })

    // Using client size values, because when a webview is transformed `newSize`
    // is incorrect
    newSize.width = this.webviewNode.clientWidth
    newSize.height = this.webviewNode.clientHeight

    resizeEvent.newWidth = newSize.width
    resizeEvent.newHeight = newSize.height
    this.dispatchEvent(resizeEvent)
    if (this.guestInstanceId) {
      return guestViewInternal.setSize(this.guestInstanceId, {
        normal: newSize
      })
    }
  }

  WebViewImpl.prototype.createGuest = function () {
    return guestViewInternal.createGuest(this.buildParams(), (event, guestInstanceId) => {
      this.attachWindow(guestInstanceId)
    })
  }

  WebViewImpl.prototype.dispatchEvent = function (webViewEvent) {
    return this.webviewNode.dispatchEvent(webViewEvent)
  }

  // Adds an 'on<event>' property on the webview, which can be used to set/unset
  // an event handler.
  WebViewImpl.prototype.setupEventProperty = function (eventName) {
    var propertyName
    propertyName = 'on' + eventName.toLowerCase()
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
  WebViewImpl.prototype.onLoadCommit = function (webViewEvent) {
    var newValue, oldValue
    oldValue = this.webviewNode.getAttribute(webViewConstants.ATTRIBUTE_SRC)
    newValue = webViewEvent.url
    if (webViewEvent.isMainFrame && (oldValue !== newValue)) {
      // Touching the src attribute triggers a navigation. To avoid
      // triggering a page reload on every guest-initiated navigation,
      // we do not handle this mutation.
      return this.attributes[webViewConstants.ATTRIBUTE_SRC].setValueIgnoreMutation(newValue)
    }
  }

  WebViewImpl.prototype.onAttach = function (storagePartitionId) {
    return this.attributes[webViewConstants.ATTRIBUTE_PARTITION].setValue(storagePartitionId)
  }

  WebViewImpl.prototype.buildParams = function () {
    var attribute, attributeName, css, elementRect, params, ref1
    params = {
      instanceId: this.viewInstanceId,
      userAgentOverride: this.userAgentOverride,
      zoomFactor: webFrame.getZoomFactor()
    }
    ref1 = this.attributes
    for (attributeName in ref1) {
      if (!hasProp.call(ref1, attributeName)) continue
      attribute = ref1[attributeName]
      params[attributeName] = attribute.getValue()
    }

    // When the WebView is not participating in layout (display:none)
    // then getBoundingClientRect() would report a width and height of 0.
    // However, in the case where the WebView has a fixed size we can
    // use that value to initially size the guest so as to avoid a relayout of
    // the on display:block.
    css = window.getComputedStyle(this.webviewNode, null)
    elementRect = this.webviewNode.getBoundingClientRect()
    params.elementWidth = parseInt(elementRect.width) || parseInt(css.getPropertyValue('width'))
    params.elementHeight = parseInt(elementRect.height) || parseInt(css.getPropertyValue('height'))
    return params
  }

  WebViewImpl.prototype.attachWindow = function (guestInstanceId) {
    this.guestInstanceId = guestInstanceId
    this.webContents = remote.getGuestWebContents(this.guestInstanceId)
    if (!this.internalInstanceId) {
      return true
    }
    return guestViewInternal.attachGuest(this.internalInstanceId, this.guestInstanceId, this.buildParams())
  }

  return WebViewImpl
})()

// Registers browser plugin <object> custom element.
var registerBrowserPluginElement = function () {
  var proto = Object.create(HTMLObjectElement.prototype)
  proto.createdCallback = function () {
    this.setAttribute('type', 'application/browser-plugin')
    this.setAttribute('id', 'browser-plugin-' + getNextId())

    // The <object> node fills in the <webview> container.
    this.style.flex = '1 1 auto'
  }
  proto.attributeChangedCallback = function (name, oldValue, newValue) {
    var internal
    internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal) {
      return
    }
    return internal.handleBrowserPluginAttributeMutation(name, oldValue, newValue)
  }
  proto.attachedCallback = function () {
    // Load the plugin immediately.
    return this.nonExistentAttribute
  }
  WebViewImpl.BrowserPlugin = webFrame.registerEmbedderCustomElement('browserplugin', {
    'extends': 'object',
    prototype: proto
  })
  delete proto.createdCallback
  delete proto.attachedCallback
  delete proto.detachedCallback
  return delete proto.attributeChangedCallback
}

// Registers <webview> custom element.
var registerWebViewElement = function () {
  var createBlockHandler, createNonBlockHandler, i, j, len, len1, m, methods, nonblockMethods, proto
  proto = Object.create(HTMLObjectElement.prototype)
  proto.createdCallback = function () {
    return new WebViewImpl(this)
  }
  proto.attributeChangedCallback = function (name, oldValue, newValue) {
    var internal
    internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal) {
      return
    }
    return internal.handleWebviewAttributeMutation(name, oldValue, newValue)
  }
  proto.detachedCallback = function () {
    var internal
    internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal) {
      return
    }
    guestViewInternal.deregisterEvents(internal.viewInstanceId)
    internal.elementAttached = false
    return internal.reset()
  }
  proto.attachedCallback = function () {
    var internal
    internal = v8Util.getHiddenValue(this, 'internal')
    if (!internal) {
      return
    }
    if (!internal.elementAttached) {
      guestViewInternal.registerEvents(internal, internal.viewInstanceId)
      internal.elementAttached = true
      return internal.attributes[webViewConstants.ATTRIBUTE_SRC].parse()
    }
  }

  // Public-facing API methods.
  methods = [
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
    'capturePage'
  ]
  nonblockMethods = [
    'insertCSS',
    'insertText',
    'send',
    'sendInputEvent',
    'setZoomFactor',
    'setZoomLevel',
    'setZoomLevelLimits'
  ]

  // Forward proto.foo* method calls to WebViewImpl.foo*.
  createBlockHandler = function (m) {
    return function (...args) {
      const internal = v8Util.getHiddenValue(this, 'internal')
      if (internal.webContents) {
        return internal.webContents[m].apply(internal.webContents, args)
      } else {
        throw new Error(`Cannot call ${m} because the webContents is unavailable. The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.`)
      }
    }
  }
  for (i = 0, len = methods.length; i < len; i++) {
    m = methods[i]
    proto[m] = createBlockHandler(m)
  }
  createNonBlockHandler = function (m) {
    return function (...args) {
      const internal = v8Util.getHiddenValue(this, 'internal')
      return ipcRenderer.send.apply(ipcRenderer, ['ELECTRON_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', null, internal.guestInstanceId, m].concat(args))
    }
  }
  for (j = 0, len1 = nonblockMethods.length; j < len1; j++) {
    m = nonblockMethods[j]
    proto[m] = createNonBlockHandler(m)
  }

  proto.executeJavaScript = function (code, hasUserGesture, callback) {
    var internal = v8Util.getHiddenValue(this, 'internal')
    if (typeof hasUserGesture === 'function') {
      callback = hasUserGesture
      hasUserGesture = false
    }
    let requestId = getNextId()
    ipcRenderer.send('ELECTRON_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', requestId, internal.guestInstanceId, 'executeJavaScript', code, hasUserGesture)
    ipcRenderer.once(`ELECTRON_RENDERER_ASYNC_CALL_TO_GUEST_VIEW_RESPONSE_${requestId}`, function (event, result) {
      if (callback) callback(result)
    })
  }

  // WebContents associated with this webview.
  proto.getWebContents = function () {
    var internal = v8Util.getHiddenValue(this, 'internal')
    return internal.webContents
  }

  window.WebView = webFrame.registerEmbedderCustomElement('webview', {
    prototype: proto
  })

  // Delete the callbacks so developers cannot call them and produce unexpected
  // behavior.
  delete proto.createdCallback
  delete proto.attachedCallback
  delete proto.detachedCallback
  return delete proto.attributeChangedCallback
}

var useCapture = true

var listener = function (event) {
  if (document.readyState === 'loading') {
    return
  }
  registerBrowserPluginElement()
  registerWebViewElement()
  return window.removeEventListener(event.type, listener, useCapture)
}

window.addEventListener('readystatechange', listener, true)

module.exports = WebViewImpl
