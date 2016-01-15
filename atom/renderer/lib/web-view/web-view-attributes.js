const WebViewImpl = require('./web-view');
const guestViewInternal = require('./guest-view-internal');
const webViewConstants = require('./web-view-constants');
const remote = require('electron').remote;
var util = require('util');

// Helper function to resolve url set in attribute.
var a = document.createElement('a');

var resolveURL = function(url) {
  a.href = url;
  return a.href;
};

// Attribute objects.
// Default implementation of a WebView attribute.
function WebViewAttribute(name, webViewImpl) {
  this.name = name;
  this.value = webViewImpl.webviewNode[name] || '';
  this.webViewImpl = webViewImpl;
  this.ignoreMutation = false;
  this.defineProperty();
}

// Retrieves and returns the attribute's value.
WebViewAttribute.prototype.getValue = function() {
  return this.webViewImpl.webviewNode.getAttribute(this.name) || this.value;
};

// Sets the attribute's value.
WebViewAttribute.prototype.setValue = function(value) {
  return this.webViewImpl.webviewNode.setAttribute(this.name, value || '');
};

// Changes the attribute's value without triggering its mutation handler.
WebViewAttribute.prototype.setValueIgnoreMutation = function(value) {
  this.ignoreMutation = true;
  this.setValue(value);
  return this.ignoreMutation = false;
};

// Defines this attribute as a property on the webview node.
WebViewAttribute.prototype.defineProperty = function() {
  return Object.defineProperty(this.webViewImpl.webviewNode, this.name, {
    get: (function(_this) {
      return function() {
        return _this.getValue();
      };
    })(this),
    set: (function(_this) {
      return function(value) {
        return _this.setValue(value);
      };
    })(this),
    enumerable: true
  });
};

// Called when the attribute's value changes.
WebViewAttribute.prototype.handleMutation = function() {};

// An attribute that is treated as a Boolean.
function BooleanAttribute(name, webViewImpl) {
  BooleanAttribute.super_.call(this, name, webViewImpl)
}

util.inherits(BooleanAttribute, WebViewAttribute);

BooleanAttribute.prototype.getValue = function() {
  return this.webViewImpl.webviewNode.hasAttribute(this.name);
};

BooleanAttribute.prototype.setValue = function(value) {
  if (!value) {
    return this.webViewImpl.webviewNode.removeAttribute(this.name);
  } else {
    return this.webViewImpl.webviewNode.setAttribute(this.name, '');
  }
};

// Attribute that specifies whether transparency is allowed in the webview.
function AllowTransparencyAttribute(webViewImpl) {
  AllowTransparencyAttribute.super_.call(this, webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY, webViewImpl);
}

util.inherits(AllowTransparencyAttribute, BooleanAttribute);

AllowTransparencyAttribute.prototype.handleMutation = function(oldValue, newValue) {
  if (!this.webViewImpl.guestInstanceId) {
    return;
  }
  return guestViewInternal.setAllowTransparency(this.webViewImpl.guestInstanceId, this.getValue());
};

// Attribute used to define the demension limits of autosizing.
function AutosizeDimensionAttribute(name, webViewImpl) {
  AutosizeDimensionAttribute.super_.call(this, name, webViewImpl);
}

util.inherits(AutosizeDimensionAttribute, WebViewAttribute);

AutosizeDimensionAttribute.prototype.getValue = function() {
  return parseInt(this.webViewImpl.webviewNode.getAttribute(this.name)) || 0;
};

AutosizeDimensionAttribute.prototype.handleMutation = function(oldValue, newValue) {
  if (!this.webViewImpl.guestInstanceId) {
    return;
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
  });
};

// Attribute that specifies whether the webview should be autosized.
function AutosizeAttribute(webViewImpl) {
  AutosizeAttribute.super_.call(this, webViewConstants.ATTRIBUTE_AUTOSIZE, webViewImpl);
}

util.inherits(AutosizeAttribute, BooleanAttribute);

AutosizeAttribute.prototype.handleMutation = AutosizeDimensionAttribute.prototype.handleMutation;

// Attribute representing the state of the storage partition.
function PartitionAttribute(webViewImpl) {
  PartitionAttribute.super_.call(this, webViewConstants.ATTRIBUTE_PARTITION, webViewImpl);
  this.validPartitionId = true;
}

util.inherits(PartitionAttribute, WebViewAttribute);

PartitionAttribute.prototype.handleMutation = function(oldValue, newValue) {
  newValue = newValue || '';

  // The partition cannot change if the webview has already navigated.
  if (!this.webViewImpl.beforeFirstNavigation) {
    window.console.error(webViewConstants.ERROR_MSG_ALREADY_NAVIGATED);
    this.setValueIgnoreMutation(oldValue);
    return;
  }
  if (newValue === 'persist:') {
    this.validPartitionId = false;
    return window.console.error(webViewConstants.ERROR_MSG_INVALID_PARTITION_ATTRIBUTE);
  }
};

// Attribute that handles the location and navigation of the webview.
function SrcAttribute(webViewImpl) {
  SrcAttribute.super_.call(this, webViewConstants.ATTRIBUTE_SRC, webViewImpl);
  this.setupMutationObserver();
}

util.inherits(SrcAttribute, WebViewAttribute);

SrcAttribute.prototype.getValue = function() {
  if (this.webViewImpl.webviewNode.hasAttribute(this.name)) {
    return resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name));
  } else {
    return this.value;
  }
};

SrcAttribute.prototype.setValueIgnoreMutation = function(value) {
  WebViewAttribute.prototype.setValueIgnoreMutation.call(this, value);

  // takeRecords() is needed to clear queued up src mutations. Without it, it
  // is possible for this change to get picked up asyncronously by src's
  // mutation observer |observer|, and then get handled even though we do not
  // want to handle this mutation.
  return this.observer.takeRecords();
};

SrcAttribute.prototype.handleMutation = function(oldValue, newValue) {

  // Once we have navigated, we don't allow clearing the src attribute.
  // Once <webview> enters a navigated state, it cannot return to a
  // placeholder state.
  if (!newValue && oldValue) {

    // src attribute changes normally initiate a navigation. We suppress
    // the next src attribute handler call to avoid reloading the page
    // on every guest-initiated navigation.
    this.setValueIgnoreMutation(oldValue);
    return;
  }
  return this.parse();
};

// The purpose of this mutation observer is to catch assignment to the src
// attribute without any changes to its value. This is useful in the case
// where the webview guest has crashed and navigating to the same address
// spawns off a new process.
SrcAttribute.prototype.setupMutationObserver = function() {
  var params;
  this.observer = new MutationObserver((function(_this) {
    return function(mutations) {
      var i, len, mutation, newValue, oldValue;
      for (i = 0, len = mutations.length; i < len; i++) {
        mutation = mutations[i];
        oldValue = mutation.oldValue;
        newValue = _this.getValue();
        if (oldValue !== newValue) {
          return;
        }
        _this.handleMutation(oldValue, newValue);
      }
    };
  })(this));
  params = {
    attributes: true,
    attributeOldValue: true,
    attributeFilter: [this.name]
  };
  return this.observer.observe(this.webViewImpl.webviewNode, params);
};

SrcAttribute.prototype.parse = function() {
  var guestContents, httpreferrer, opts, useragent;
  if (!this.webViewImpl.elementAttached || !this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_PARTITION].validPartitionId || !this.getValue()) {
    return;
  }
  if (this.webViewImpl.guestInstanceId == null) {
    if (this.webViewImpl.beforeFirstNavigation) {
      this.webViewImpl.beforeFirstNavigation = false;
      this.webViewImpl.createGuest();
    }
    return;
  }

  // Navigate to |this.src|.
  opts = {};
  httpreferrer = this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_HTTPREFERRER].getValue();
  if (httpreferrer) {
    opts.httpReferrer = httpreferrer;
  }
  useragent = this.webViewImpl.attributes[webViewConstants.ATTRIBUTE_USERAGENT].getValue();
  if (useragent) {
    opts.userAgent = useragent;
  }
  guestContents = remote.getGuestWebContents(this.webViewImpl.guestInstanceId);
  return guestContents.loadURL(this.getValue(), opts);
};

// Attribute specifies HTTP referrer.
function HttpReferrerAttribute(webViewImpl) {
  HttpReferrerAttribute.super_.call(this, webViewConstants.ATTRIBUTE_HTTPREFERRER, webViewImpl);
}

util.inherits(HttpReferrerAttribute, WebViewAttribute);

// Attribute specifies user agent
function UserAgentAttribute(webViewImpl) {
  UserAgentAttribute.super_.call(this, webViewConstants.ATTRIBUTE_USERAGENT, webViewImpl);
}

util.inherits(UserAgentAttribute, WebViewAttribute);

// Attribute that set preload script.
function PreloadAttribute(webViewImpl) {
  PreloadAttribute.super_.call(this, webViewConstants.ATTRIBUTE_PRELOAD, webViewImpl);
}

util.inherits(PreloadAttribute, WebViewAttribute);

PreloadAttribute.prototype.getValue = function() {
  var preload, protocol;
  if (!this.webViewImpl.webviewNode.hasAttribute(this.name)) {
    return this.value;
  }
  preload = resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name));
  protocol = preload.substr(0, 5);
  if (protocol !== 'file:') {
    console.error(webViewConstants.ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE);
    preload = '';
  }
  return preload;
};

// Sets up all of the webview attributes.
WebViewImpl.prototype.setupWebViewAttributes = function() {
  var attribute, autosizeAttributes, i, len, results;
  this.attributes = {};
  this.attributes[webViewConstants.ATTRIBUTE_ALLOWTRANSPARENCY] = new AllowTransparencyAttribute(this);
  this.attributes[webViewConstants.ATTRIBUTE_AUTOSIZE] = new AutosizeAttribute(this);
  this.attributes[webViewConstants.ATTRIBUTE_PARTITION] = new PartitionAttribute(this);
  this.attributes[webViewConstants.ATTRIBUTE_SRC] = new SrcAttribute(this);
  this.attributes[webViewConstants.ATTRIBUTE_HTTPREFERRER] = new HttpReferrerAttribute(this);
  this.attributes[webViewConstants.ATTRIBUTE_USERAGENT] = new UserAgentAttribute(this);
  this.attributes[webViewConstants.ATTRIBUTE_NODEINTEGRATION] = new BooleanAttribute(webViewConstants.ATTRIBUTE_NODEINTEGRATION, this);
  this.attributes[webViewConstants.ATTRIBUTE_PLUGINS] = new BooleanAttribute(webViewConstants.ATTRIBUTE_PLUGINS, this);
  this.attributes[webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY] = new BooleanAttribute(webViewConstants.ATTRIBUTE_DISABLEWEBSECURITY, this);
  this.attributes[webViewConstants.ATTRIBUTE_ALLOWPOPUPS] = new BooleanAttribute(webViewConstants.ATTRIBUTE_ALLOWPOPUPS, this);
  this.attributes[webViewConstants.ATTRIBUTE_PRELOAD] = new PreloadAttribute(this);
  autosizeAttributes = [webViewConstants.ATTRIBUTE_MAXHEIGHT, webViewConstants.ATTRIBUTE_MAXWIDTH, webViewConstants.ATTRIBUTE_MINHEIGHT, webViewConstants.ATTRIBUTE_MINWIDTH];
  results = [];
  for (i = 0, len = autosizeAttributes.length; i < len; i++) {
    attribute = autosizeAttributes[i];
    results.push(this.attributes[attribute] = new AutosizeDimensionAttribute(attribute, this));
  }
  return results;
};
