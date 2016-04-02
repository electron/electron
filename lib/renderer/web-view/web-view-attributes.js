'use strict'

const WebViewImpl = require('./web-view')
const guestViewInternal = require('./guest-view-internal')
const webViewConstants = require('./web-view-constants')
const remote = require('electron').remote

// Helper function to resolve url set in attribute.
var a = document.createElement('a')

var resolveURL = function (url) {
  a.href = url
  return a.href
}

// Attribute objects.
// Default implementation of a WebView attribute.
class WebViewAttribute {
  constructor (name, webViewImpl) {
    this.name = name
    this.value = webViewImpl.webviewNode[name] || ''
    this.webViewImpl = webViewImpl
    this.ignoreMutation = false
    this.defineProperty()
  }

  // Retrieves and returns the attribute's value.
  getValue () {
    return this.webViewImpl.webviewNode.getAttribute(this.name) || this.value
  }

  // Sets the attribute's value.
  setValue (value) {
    return this.webViewImpl.webviewNode.setAttribute(this.name, value || '')
  }

  // Changes the attribute's value without triggering its mutation handler.
  setValueIgnoreMutation (value) {
    this.ignoreMutation = true
    this.setValue(value)
    this.ignoreMutation = false
  }

  // Defines this attribute as a property on the webview node.
  defineProperty () {
    return Object.defineProperty(this.webViewImpl.webviewNode, this.name, {
      get: () => {
        return this.getValue()
      },
      set: (value) => {
        return this.setValue(value)
      },
      enumerable: true
    })
  }

  // Called when the attribute's value changes.
  handleMutation () {}
}

// An attribute that is treated as a Boolean.
class BooleanAttribute extends WebViewAttribute {
  getValue () {
    return this.webViewImpl.webviewNode.hasAttribute(this.name)
  }

  setValue (value) {
    if (!value) {
      return this.webViewImpl.webviewNode.removeAttribute(this.name)
    } else {
      return this.webViewImpl.webviewNode.setAttribute(this.name, '')
    }
  }
}

// Attribute used to define the demension limits of autosizing.
class AutosizeDimensionAttribute extends WebViewAttribute {
  getValue () {
    return parseInt(this.webViewImpl.webviewNode.getAttribute(this.name)) || 0
  }

  handleMutation () {
    if (!this.webViewImpl.guestInstanceId) {
      return
    }
    return guestViewInternal.setSize(this.webViewImpl.guestInstanceId, {
      enableAutoSize: this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_AUTOSIZE].getValue(),
      min: {
        width: parseInt(this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_MINWIDTH].getValue() || 0),
        height: parseInt(this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_MINHEIGHT].getValue() || 0)
      },
      max: {
        width: parseInt(this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_MAXWIDTH].getValue() || 0),
        height: parseInt(this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_MAXHEIGHT].getValue() || 0)
      }
    })
  }
}

// Attribute that specifies whether the webview should be autosized.
class AutosizeAttribute extends BooleanAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_AUTOSIZE, webViewImpl)
  }
}

AutosizeAttribute.prototype.handleMutation = AutosizeDimensionAttribute.prototype.handleMutation

// Attribute representing the state of the storage partition.
class PartitionAttribute extends WebViewAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_PARTITION, webViewImpl)
    this.validPartitionId = true
  }

  handleMutation (oldValue, newValue) {
    newValue = newValue || ''

    // The partition cannot change if the webview has already navigated.
    if (!this.webViewImpl.beforeFirstNavigation) {
      window.console.error(webViewConstants.ERROR_MSG_ALREADY_NAVIGATED)
      this.setValueIgnoreMutation(oldValue)
      return
    }
    if (newValue === 'persist:') {
      this.validPartitionId = false
      return window.console.error(webViewConstants.ERROR_MSG_INVALID_PARTITION_ATTRIBUTE)
    }
  }
}

// Attribute that handles the location and navigation of the webview.
class SrcAttribute extends WebViewAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_SRC, webViewImpl)
    this.setupMutationObserver()
  }

  getValue () {
    if (this.webViewImpl.webviewNode.hasAttribute(this.name)) {
      return resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name))
    } else {
      return this.value
    }
  }

  setValueIgnoreMutation (value) {
    super.setValueIgnoreMutation(value)

    // takeRecords() is needed to clear queued up src mutations. Without it, it
    // is possible for this change to get picked up asyncronously by src's
    // mutation observer |observer|, and then get handled even though we do not
    // want to handle this mutation.
    return this.observer.takeRecords()
  }

  handleMutation (oldValue, newValue) {
    // Once we have navigated, we don't allow clearing the src attribute.
    // Once <webview> enters a navigated state, it cannot return to a
    // placeholder state.
    if (!newValue && oldValue) {
      // src attribute changes normally initiate a navigation. We suppress
      // the next src attribute handler call to avoid reloading the page
      // on every guest-initiated navigation.
      this.setValueIgnoreMutation(oldValue)
      return
    }
    return this.parse()
  }

  // The purpose of this mutation observer is to catch assignment to the src
  // attribute without any changes to its value. This is useful in the case
  // where the webview guest has crashed and navigating to the same address
  // spawns off a new process.
  setupMutationObserver () {
    var params
    this.observer = new MutationObserver((mutations) => {
      var i, len, mutation, newValue, oldValue
      for (i = 0, len = mutations.length; i < len; i++) {
        mutation = mutations[i]
        oldValue = mutation.oldValue
        newValue = this.getValue()
        if (oldValue !== newValue) {
          return
        }
        this.handleMutation(oldValue, newValue)
      }
    })
    params = {
      attributes: true,
      attributeOldValue: true,
      attributeFilter: [this.name]
    }
    return this.observer.observe(this.webViewImpl.webviewNode, params)
  }

  parse () {
    var guestContents, httpreferrer, opts, useragent
    if (!this.webViewImpl.elementAttached || !this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_PARTITION].validPartitionId || !this.getValue()) {
      return
    }
    if (this.webViewImpl.guestInstanceId == null) {
      if (this.webViewImpl.beforeFirstNavigation) {
        this.webViewImpl.beforeFirstNavigation = false
        this.webViewImpl.createGuest()
      }
      return
    }

    // Navigate to |this.src|.
    opts = {}
    httpreferrer = this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_HTTPREFERRER].getValue()
    if (httpreferrer) {
      opts.httpReferrer = httpreferrer
    }
    useragent = this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_USERAGENT].getValue()
    if (useragent) {
      opts.userAgent = useragent
    }
    guestContents = remote.getGuestWebContents(this.webViewImpl.guestInstanceId)
    return guestContents.loadURL(this.getValue(), opts)
  }
}

// Attribute specifies HTTP referrer.
class HttpReferrerAttribute extends WebViewAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_HTTPREFERRER, webViewImpl)
  }
}

// Attribute specifies user agent
class UserAgentAttribute extends WebViewAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_USERAGENT, webViewImpl)
  }
}

// Attribute that set preload script.
class PreloadAttribute extends WebViewAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_PRELOAD, webViewImpl)
  }

  getValue () {
    var preload, protocol
    if (!this.webViewImpl.webviewNode.hasAttribute(this.name)) {
      return this.value
    }
    preload = resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name))
    protocol = preload.substr(0, 5)
    if (protocol !== 'file:') {
      console.error(webViewConstants.ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE)
      preload = ''
    }
    return preload
  }
}

// Attribute that specifies the blink features to be enabled.
class BlinkFeaturesAttribute extends WebViewAttribute {
  constructor (webViewImpl) {
    super(webViewConstants.ATTRIBUTE_BLINKFEATURES, webViewImpl)
  }
}

// Sets up all of the webview attributes.
WebViewImpl.prototype.setupWebViewAttributes = function () {
  this.attributes = {}
  this.attributes[webViewConstants.ATTRIBUTE_AUTOSIZE] = new AutosizeAttribute(this)
  this.attributes[webViewConstants.ATTRIBUTE_PARTITION] = new PartitionAttribute(this)
  this.attributes[webViewConstants.ATTRIBUTE_SRC] = new SrcAttribute(this)
  this.attributes[webViewConstants.ATTRIBUTE_HTTPREFERRER] = new HttpReferrerAttribute(this)
  this.attributes[webViewConstants.ATTRIBUTE_USERAGENT] = new UserAgentAttribute(this)
  this.attributes[webViewConstants.ATTRIBUTE_NODEINTEGRATION] = new BooleanAttribute(webViewConstants.ATTRIBUTE_NODEINTEGRATION, this)
  this.attributes[webViewConstants.ATTRIBUTE_PLUGINS] = new BooleanAttribute(webViewConstants.ATTRIBUTE_PLUGINS, this)
  this.attributes[webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY] = new BooleanAttribute(webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY, this)
  this.attributes[webViewConstants.ATTRIBUTE_ALLOWPOPUPS] = new BooleanAttribute(webViewConstants.ATTRIBUTE_ALLOWPOPUPS, this)
  this.attributes[webViewConstants.ATTRIBUTE_PRELOAD] = new PreloadAttribute(this)
  this.attributes[webViewConstants.ATTRIBUTE_BLINKFEATURES] = new BlinkFeaturesAttribute(this)

  const autosizeAttributes = [webViewConstants.ATTRIBUTE_MAXHEIGHT, webViewConstants.ATTRIBUTE_MAXWIDTH, webViewConstants.ATTRIBUTE_MINHEIGHT, webViewConstants.ATTRIBUTE_MINWIDTH]
  autosizeAttributes.forEach((attribute) => {
    this.attributes[attribute] = new AutosizeDimensionAttribute(attribute, this)
  })
}
