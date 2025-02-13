import { WEB_VIEW_ATTRIBUTES, WEB_VIEW_ERROR_MESSAGES } from '@electron/internal/renderer/web-view/web-view-constants';
import type { WebViewImpl } from '@electron/internal/renderer/web-view/web-view-impl';

const resolveURL = function (url?: string | null) {
  return url ? new URL(url, location.href).href : '';
};

interface MutationHandler {
  handleMutation (_oldValue: any, _newValue: any): any;
}

// Attribute objects.
// Default implementation of a WebView attribute.
export class WebViewAttribute implements MutationHandler {
  public value: any;
  public ignoreMutation = false;

  constructor (public name: string, public webViewImpl: WebViewImpl) {
    this.name = name;
    this.value = (webViewImpl.webviewNode as Record<string, any>)[name] || '';
    this.webViewImpl = webViewImpl;
    this.defineProperty();
  }

  // Retrieves and returns the attribute's value.
  public getValue () {
    return this.webViewImpl.webviewNode.getAttribute(this.name) || this.value;
  }

  // Sets the attribute's value.
  public setValue (value: any) {
    this.webViewImpl.webviewNode.setAttribute(this.name, value || '');
  }

  // Changes the attribute's value without triggering its mutation handler.
  public setValueIgnoreMutation (value: any) {
    this.ignoreMutation = true;
    this.setValue(value);
    this.ignoreMutation = false;
  }

  // Defines this attribute as a property on the webview node.
  public defineProperty () {
    return Object.defineProperty(this.webViewImpl.webviewNode, this.name, {
      get: () => {
        return this.getValue();
      },
      set: (value) => {
        return this.setValue(value);
      },
      enumerable: true
    });
  }

  // Called when the attribute's value changes.
  public handleMutation: MutationHandler['handleMutation'] = () => undefined;
}

// An attribute that is treated as a Boolean.
class BooleanAttribute extends WebViewAttribute {
  getValue () {
    return this.webViewImpl.webviewNode.hasAttribute(this.name);
  }

  setValue (value: boolean) {
    if (value) {
      this.webViewImpl.webviewNode.setAttribute(this.name, '');
    } else {
      this.webViewImpl.webviewNode.removeAttribute(this.name);
    }
  }
}

// Attribute representing the state of the storage partition.
export class PartitionAttribute extends WebViewAttribute {
  public validPartitionId = true;

  constructor (public webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.PARTITION, webViewImpl);
  }

  public handleMutation = (oldValue: any, newValue: any) => {
    newValue = newValue || '';

    // The partition cannot change if the webview has already navigated.
    if (!this.webViewImpl.beforeFirstNavigation) {
      console.error(WEB_VIEW_ERROR_MESSAGES.ALREADY_NAVIGATED);
      this.setValueIgnoreMutation(oldValue);
      return;
    }
    if (newValue === 'persist:') {
      this.validPartitionId = false;
      console.error(WEB_VIEW_ERROR_MESSAGES.INVALID_PARTITION_ATTRIBUTE);
    }
  };
}

// Attribute that handles the location and navigation of the webview.
export class SrcAttribute extends WebViewAttribute {
  public observer!: MutationObserver;

  constructor (public webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.SRC, webViewImpl);
    this.setupMutationObserver();
  }

  public getValue () {
    if (this.webViewImpl.webviewNode.hasAttribute(this.name)) {
      return resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name));
    } else {
      return this.value;
    }
  }

  public setValueIgnoreMutation (value: any) {
    super.setValueIgnoreMutation(value);

    // takeRecords() is needed to clear queued up src mutations. Without it, it
    // is possible for this change to get picked up asynchronously by src's
    // mutation observer |observer|, and then get handled even though we do not
    // want to handle this mutation.
    this.observer.takeRecords();
  }

  public handleMutation = (oldValue: any, newValue: any) => {
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
    this.parse();
  };

  // The purpose of this mutation observer is to catch assignment to the src
  // attribute without any changes to its value. This is useful in the case
  // where the webview guest has crashed and navigating to the same address
  // spawns off a new process.
  public setupMutationObserver () {
    this.observer = new MutationObserver((mutations) => {
      for (const mutation of mutations) {
        const { oldValue } = mutation;
        const newValue = this.getValue();
        if (oldValue !== newValue) {
          return;
        }
        this.handleMutation(oldValue, newValue);
      }
    });

    const params = {
      attributes: true,
      attributeOldValue: true,
      attributeFilter: [this.name]
    };

    this.observer.observe(this.webViewImpl.webviewNode, params);
  }

  public parse () {
    if (!this.webViewImpl.elementAttached || !(this.webViewImpl.attributes.get(WEB_VIEW_ATTRIBUTES.PARTITION) as PartitionAttribute).validPartitionId || !this.getValue()) {
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
    const opts: Record<string, string> = {};

    const httpreferrer = this.webViewImpl.attributes.get(WEB_VIEW_ATTRIBUTES.HTTPREFERRER)!.getValue();
    if (httpreferrer) {
      opts.httpReferrer = httpreferrer;
    }

    const useragent = this.webViewImpl.attributes.get(WEB_VIEW_ATTRIBUTES.USERAGENT)!.getValue();
    if (useragent) {
      opts.userAgent = useragent;
    }

    (this.webViewImpl.webviewNode as Electron.WebviewTag).loadURL(this.getValue(), opts)
      .catch(err => {
        console.error('Unexpected error while loading URL', err);
      });
  }
}

// Attribute specifies HTTP referrer.
class HttpReferrerAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.HTTPREFERRER, webViewImpl);
  }
}

// Attribute specifies user agent
class UserAgentAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.USERAGENT, webViewImpl);
  }
}

// Attribute that set preload script.
class PreloadAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.PRELOAD, webViewImpl);
  }

  public getValue () {
    if (!this.webViewImpl.webviewNode.hasAttribute(this.name)) {
      return this.value;
    }

    let preload = resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name));
    const protocol = preload.substr(0, 5);

    if (protocol !== 'file:') {
      console.error(WEB_VIEW_ERROR_MESSAGES.INVALID_PRELOAD_ATTRIBUTE);
      preload = '';
    }

    return preload;
  }
}

// Attribute that specifies the blink features to be enabled.
class BlinkFeaturesAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.BLINKFEATURES, webViewImpl);
  }
}

// Attribute that specifies the blink features to be disabled.
class DisableBlinkFeaturesAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.DISABLEBLINKFEATURES, webViewImpl);
  }
}

// Attribute that specifies the web preferences to be enabled.
class WebPreferencesAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_ATTRIBUTES.WEBPREFERENCES, webViewImpl);
  }
}

// Sets up all of the webview attributes.
export function setupWebViewAttributes (self: WebViewImpl) {
  return new Map<string, WebViewAttribute>([
    [WEB_VIEW_ATTRIBUTES.PARTITION, new PartitionAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.SRC, new SrcAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.HTTPREFERRER, new HttpReferrerAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.USERAGENT, new UserAgentAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.NODEINTEGRATION, new BooleanAttribute(WEB_VIEW_ATTRIBUTES.NODEINTEGRATION, self)],
    [WEB_VIEW_ATTRIBUTES.NODEINTEGRATIONINSUBFRAMES, new BooleanAttribute(WEB_VIEW_ATTRIBUTES.NODEINTEGRATIONINSUBFRAMES, self)],
    [WEB_VIEW_ATTRIBUTES.PLUGINS, new BooleanAttribute(WEB_VIEW_ATTRIBUTES.PLUGINS, self)],
    [WEB_VIEW_ATTRIBUTES.DISABLEWEBSECURITY, new BooleanAttribute(WEB_VIEW_ATTRIBUTES.DISABLEWEBSECURITY, self)],
    [WEB_VIEW_ATTRIBUTES.ALLOWPOPUPS, new BooleanAttribute(WEB_VIEW_ATTRIBUTES.ALLOWPOPUPS, self)],
    [WEB_VIEW_ATTRIBUTES.PRELOAD, new PreloadAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.BLINKFEATURES, new BlinkFeaturesAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.DISABLEBLINKFEATURES, new DisableBlinkFeaturesAttribute(self)],
    [WEB_VIEW_ATTRIBUTES.WEBPREFERENCES, new WebPreferencesAttribute(self)]
  ]);
}
