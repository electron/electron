import { syncMethods, asyncMethods, properties } from '@electron/internal/common/web-view-methods';
import type * as guestViewInternalModule from '@electron/internal/renderer/web-view/guest-view-internal';
import type { WebViewAttribute, PartitionAttribute } from '@electron/internal/renderer/web-view/web-view-attributes';
import { setupWebViewAttributes } from '@electron/internal/renderer/web-view/web-view-attributes';
import { WEB_VIEW_ATTRIBUTES } from '@electron/internal/renderer/web-view/web-view-constants';

// ID generator.
let nextId = 0;

const getNextId = function () {
  return ++nextId;
};

export interface WebViewImplHooks {
  readonly guestViewInternal: typeof guestViewInternalModule;
  readonly allowGuestViewElementDefinition: NodeJS.InternalWebFrame['allowGuestViewElementDefinition'];
  readonly setIsWebView?: (iframe: HTMLIFrameElement) => void;
}

// Represents the internal state of the WebView node.
export class WebViewImpl {
  public beforeFirstNavigation = true;
  public elementAttached = false;
  public guestInstanceId?: number;
  public hasFocus = false;
  public internalInstanceId?: number;
  public viewInstanceId: number;

  public internalElement: HTMLEmbedElement;

  public attributes: Map<string, WebViewAttribute>;

  constructor(
    public webviewNode: HTMLElement,
    private hooks: WebViewImplHooks
  ) {
    // Create the internal plugin element that hosts the guest surface.
    this.internalElement = this.createInternalElement();
    const shadowRoot = this.webviewNode.attachShadow({ mode: 'open' });
    const style = shadowRoot.ownerDocument.createElement('style');
    style.textContent = ':host { display: flex; }';
    shadowRoot.appendChild(style);
    this.attributes = setupWebViewAttributes(this);
    this.viewInstanceId = getNextId();
    shadowRoot.appendChild(this.internalElement);
  }

  // The guest is composited by a Surface Embed plugin. The plugin is only
  // instantiated once the element has a |type|, which we set after the browser
  // has created the guest and handed back its content id.
  createInternalElement() {
    const embedElement = document.createElement('embed');
    embedElement.style.flex = '1 1 auto';
    embedElement.style.width = '100%';
    embedElement.style.border = '0';
    embedElement.tabIndex = 0;
    return embedElement;
  }

  // Resets some state upon reattaching <webview> element to the DOM.
  reset() {
    // If guestInstanceId is defined then the <webview> has navigated and has
    // already picked up a partition ID. Thus, we need to reset the initialization
    // state. However, it may be the case that beforeFirstNavigation is false BUT
    // guestInstanceId has yet to be initialized. This means that we have not
    // heard back from createGuest yet. We will not reset the flag in this case so
    // that we don't end up allocating a second guest.
    if (this.guestInstanceId) {
      this.guestInstanceId = undefined;
    }

    this.beforeFirstNavigation = true;
    (this.attributes.get(WEB_VIEW_ATTRIBUTES.PARTITION) as PartitionAttribute).validPartitionId = true;

    // A Surface Embed plugin cannot be re-pointed at a different guest, so
    // start over with a fresh element for the next attach.
    const newFrame = this.createInternalElement();
    const oldFrame = this.internalElement;
    this.internalElement = newFrame;

    if (oldFrame && oldFrame.parentNode) {
      oldFrame.parentNode.replaceChild(newFrame, oldFrame);
    }
  }

  // This observer monitors mutations to attributes of the <webview> and
  // updates the BrowserPlugin properties accordingly. In turn, updating
  // a BrowserPlugin property will update the corresponding BrowserPlugin
  // attribute, if necessary. See BrowserPlugin::UpdateDOMAttribute for more
  // details.
  handleWebviewAttributeMutation(attributeName: string, oldValue: any, newValue: any) {
    if (!this.attributes.has(attributeName) || this.attributes.get(attributeName)!.ignoreMutation) {
      return;
    }

    // Let the changed attribute handle its own mutation
    this.attributes.get(attributeName)!.handleMutation(oldValue, newValue);
  }

  createGuest() {
    this.internalInstanceId = getNextId();
    this.hooks.guestViewInternal
      .createGuest(this.internalInstanceId, this.buildParams())
      .then(({ guestInstanceId, contentId }) => {
        this.attachGuestInstance(guestInstanceId, contentId);
      });
  }

  dispatchEvent(eventName: string, props: Record<string, any> = {}) {
    const event = new Event(eventName);
    Object.assign(event, props);
    this.webviewNode.dispatchEvent(event);

    if (eventName === 'load-commit') {
      this.onLoadCommit(props);
    } else if (eventName === '-focus-change') {
      this.onFocusChange();
    } else if (eventName === 'enter-html-full-screen') {
      // Mirror element fullscreen into the embedder document, as Blink did for
      // the cross-process <iframe> this element used to wrap.
      if (!this.internalElement.matches(':fullscreen')) {
        this.internalElement
          .requestFullscreen()
          .catch((e) => console.warn('webview: mirroring guest fullscreen failed', e));
      }
    } else if (eventName === 'leave-html-full-screen') {
      const doc = this.webviewNode.ownerDocument;
      if (doc.fullscreenElement === this.webviewNode || this.internalElement.matches(':fullscreen')) {
        doc.exitFullscreen().catch(() => {});
      }
    }
  }

  // Updates state upon loadcommit.
  onLoadCommit(props: Record<string, any>) {
    const oldValue = this.webviewNode.getAttribute(WEB_VIEW_ATTRIBUTES.SRC);
    const newValue = props.url;
    if (props.isMainFrame && oldValue !== newValue) {
      // Touching the src attribute triggers a navigation. To avoid
      // triggering a page reload on every guest-initiated navigation,
      // we do not handle this mutation.
      this.attributes.get(WEB_VIEW_ATTRIBUTES.SRC)!.setValueIgnoreMutation(newValue);
    }
  }

  // Emits focus/blur events.
  onFocusChange() {
    const hasFocus = this.webviewNode.ownerDocument.activeElement === this.webviewNode;
    if (hasFocus !== this.hasFocus) {
      this.hasFocus = hasFocus;
      this.dispatchEvent(hasFocus ? 'focus' : 'blur');
    }
  }

  buildParams() {
    const params: Record<string, any> = {
      instanceId: this.viewInstanceId
    };

    for (const [attributeName, attribute] of this.attributes) {
      params[attributeName] = attribute.getValue();
    }

    return params;
  }

  attachGuestInstance(guestInstanceId: number, contentId: string) {
    if (guestInstanceId === -1) {
      this.dispatchEvent('destroyed');
      return;
    }

    if (!this.elementAttached) {
      // The element could be detached before we got response from browser.
      // Destroy the backing webContents to avoid any zombie nodes in the frame tree.
      this.hooks.guestViewInternal.detachGuest(guestInstanceId);
      return;
    }

    this.guestInstanceId = guestInstanceId;
    // Setting |type| instantiates the Surface Embed plugin, which reads
    // data-content-id and asks the browser to attach the guest.
    this.internalElement.setAttribute('data-content-id', contentId);
    this.internalElement.setAttribute('type', 'application/x-chromium-surface-embed');
  }
}

export const setupMethods = (WebViewElement: typeof ElectronInternal.WebViewElement, hooks: WebViewImplHooks) => {
  // Focusing the webview moves page focus to the plugin element, which in turn
  // focuses the guest.
  WebViewElement.prototype.focus = function (this: ElectronInternal.WebViewElement) {
    const internal = (this.shadowRoot?.querySelector('embed') ?? null) as HTMLElement | null;
    if (internal) internal.focus();
  };

  // Forward proto.foo* method calls to WebViewImpl.foo*.
  for (const method of syncMethods) {
    (WebViewElement.prototype as Record<string, any>)[method] = function (
      this: ElectronInternal.WebViewElement,
      ...args: Array<any>
    ) {
      return hooks.guestViewInternal.invokeSync(this.getWebContentsId(), method, args);
    };
  }

  for (const method of asyncMethods) {
    (WebViewElement.prototype as Record<string, any>)[method] = function (
      this: ElectronInternal.WebViewElement,
      ...args: Array<any>
    ) {
      return hooks.guestViewInternal.invoke(this.getWebContentsId(), method, args);
    };
  }

  const createPropertyGetter = function (property: string) {
    return function (this: ElectronInternal.WebViewElement) {
      return hooks.guestViewInternal.propertyGet(this.getWebContentsId(), property);
    };
  };

  const createPropertySetter = function (property: string) {
    return function (this: ElectronInternal.WebViewElement, arg: any) {
      return hooks.guestViewInternal.propertySet(this.getWebContentsId(), property, arg);
    };
  };

  for (const property of properties) {
    Object.defineProperty(WebViewElement.prototype, property, {
      get: createPropertyGetter(property),
      set: createPropertySetter(property)
    });
  }
};
