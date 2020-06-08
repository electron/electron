import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { internalContextBridge } from '@electron/internal/renderer/api/context-bridge';

const { contextIsolationEnabled } = internalContextBridge;

// This file implements the following APIs over the ctx bridge:
// - window.open()
// - window.opener.blur()
// - window.opener.close()
// - window.opener.eval()
// - window.opener.focus()
// - window.opener.location
// - window.opener.print()
// - window.opener.closed
// - window.opener.postMessage()
// - window.history.back()
// - window.history.forward()
// - window.history.go()
// - window.history.length
// - window.prompt()
// - document.hidden
// - document.visibilityState

// Helper function to resolve relative url.
const resolveURL = (url: string, base: string) => new URL(url, base).href;

// Use this method to ensure values expected as strings in the main process
// are convertible to strings in the renderer process. This ensures exceptions
// converting values to strings are thrown in this process.
const toString = (value: any) => {
  return value != null ? `${value}` : value;
};

const windowProxies = new Map<number, BrowserWindowProxy>();

const getOrCreateProxy = (guestId: number): SafelyBoundBrowserWindowProxy => {
  let proxy = windowProxies.get(guestId);
  if (proxy == null) {
    proxy = new BrowserWindowProxy(guestId);
    windowProxies.set(guestId, proxy);
  }
  return proxy.getSafe();
};

const removeProxy = (guestId: number) => {
  windowProxies.delete(guestId);
};

type LocationProperties = 'hash' | 'href' | 'host' | 'hostname' | 'origin' | 'pathname' | 'port' | 'protocol' | 'search'

class LocationProxy {
  @LocationProxy.ProxyProperty public hash!: string;
  @LocationProxy.ProxyProperty public href!: string;
  @LocationProxy.ProxyProperty public host!: string;
  @LocationProxy.ProxyProperty public hostname!: string;
  @LocationProxy.ProxyProperty public origin!: string;
  @LocationProxy.ProxyProperty public pathname!: string;
  @LocationProxy.ProxyProperty public port!: string;
  @LocationProxy.ProxyProperty public protocol!: string;
  @LocationProxy.ProxyProperty public search!: URLSearchParams;

  private guestId: number;

  /**
   * Beware: This decorator will have the _prototype_ as the `target`. It defines properties
   * commonly found in URL on the LocationProxy.
   */
  private static ProxyProperty<T> (target: LocationProxy, propertyKey: LocationProperties) {
    Object.defineProperty(target, propertyKey, {
      enumerable: true,
      configurable: true,
      get: function (this: LocationProxy): T | string {
        const guestURL = this.getGuestURL();
        const value = guestURL ? guestURL[propertyKey] : '';
        return value === undefined ? '' : value;
      },
      set: function (this: LocationProxy, newVal: T) {
        const guestURL = this.getGuestURL();
        if (guestURL) {
          // TypeScript doesn't want us to assign to read-only variables.
          // It's right, that's bad, but we're doing it anway.
          (guestURL as any)[propertyKey] = newVal;

          return this._invokeWebContentsMethod('loadURL', guestURL.toString());
        }
      }
    });
  }

  public getSafe = () => {
    const that = this;
    return {
      get href () { return that.href; },
      set href (newValue) { that.href = newValue; },
      get hash () { return that.hash; },
      set hash (newValue) { that.hash = newValue; },
      get host () { return that.host; },
      set host (newValue) { that.host = newValue; },
      get hostname () { return that.hostname; },
      set hostname (newValue) { that.hostname = newValue; },
      get origin () { return that.origin; },
      set origin (newValue) { that.origin = newValue; },
      get pathname () { return that.pathname; },
      set pathname (newValue) { that.pathname = newValue; },
      get port () { return that.port; },
      set port (newValue) { that.port = newValue; },
      get protocol () { return that.protocol; },
      set protocol (newValue) { that.protocol = newValue; },
      get search () { return that.search; },
      set search (newValue) { that.search = newValue; }
    };
  }

  constructor (guestId: number) {
    // eslint will consider the constructor "useless"
    // unless we assign them in the body. It's fine, that's what
    // TS would do anyway.
    this.guestId = guestId;
    this.getGuestURL = this.getGuestURL.bind(this);
  }

  public toString (): string {
    return this.href;
  }

  private getGuestURL (): URL | null {
    const maybeURL = this._invokeWebContentsMethodSync('getURL') as string;

    // When there's no previous frame the url will be blank, so accountfor that here
    // to prevent url parsing errors on an empty string.
    const urlString = maybeURL !== '' ? maybeURL : 'about:blank';
    try {
      return new URL(urlString);
    } catch (e) {
      console.error('LocationProxy: failed to parse string', urlString, e);
    }

    return null;
  }

  private _invokeWebContentsMethod (method: string, ...args: any[]) {
    return ipcRendererInternal.invoke('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, method, ...args);
  }

  private _invokeWebContentsMethodSync (method: string, ...args: any[]) {
    return ipcRendererUtils.invokeSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, method, ...args);
  }
}

interface SafelyBoundBrowserWindowProxy {
  location: WindowProxy['location'];
  blur: WindowProxy['blur'];
  close: WindowProxy['close'];
  eval: typeof eval; // eslint-disable-line no-eval
  focus: WindowProxy['focus'];
  print: WindowProxy['print'];
  postMessage: WindowProxy['postMessage'];
  closed: boolean;
}

class BrowserWindowProxy {
  public closed: boolean = false

  private _location: LocationProxy
  private guestId: number

  // TypeScript doesn't allow getters/accessors with different types,
  // so for now, we'll have to make do with an "any" in the mix.
  // https://github.com/Microsoft/TypeScript/issues/2521
  public get location (): LocationProxy | any {
    return this._location.getSafe();
  }

  public set location (url: string | any) {
    url = resolveURL(url, this.location.href);
    this._invokeWebContentsMethod('loadURL', url);
  }

  constructor (guestId: number) {
    this.guestId = guestId;
    this._location = new LocationProxy(guestId);

    ipcRendererInternal.once(`ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_${guestId}`, () => {
      removeProxy(guestId);
      this.closed = true;
    });
  }

  public getSafe = (): SafelyBoundBrowserWindowProxy => {
    const that = this;
    return {
      postMessage: this.postMessage,
      blur: this.blur,
      close: this.close,
      focus: this.focus,
      print: this.print,
      eval: this.eval,
      get location () {
        return that.location;
      },
      set location (url: string | any) {
        that.location = url;
      },
      get closed () {
        return that.closed;
      }
    };
  }

  public close = () => {
    this._invokeWindowMethod('destroy');
  }

  public focus = () => {
    this._invokeWindowMethod('focus');
  }

  public blur = () => {
    this._invokeWindowMethod('blur');
  }

  public print = () => {
    this._invokeWebContentsMethod('print');
  }

  public postMessage = (message: any, targetOrigin: string) => {
    ipcRendererInternal.invoke('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', this.guestId, message, toString(targetOrigin), window.location.origin);
  }

  public eval = (code: string) => {
    this._invokeWebContentsMethod('executeJavaScript', code);
  }

  private _invokeWindowMethod (method: string, ...args: any[]) {
    return ipcRendererInternal.invoke('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, method, ...args);
  }

  private _invokeWebContentsMethod (method: string, ...args: any[]) {
    return ipcRendererInternal.invoke('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, method, ...args);
  }
}

export const windowSetup = (
  guestInstanceId: number, openerId: number, isHiddenPage: boolean, usesNativeWindowOpen: boolean, rendererProcessReuseEnabled: boolean
) => {
  if (!process.sandboxed && guestInstanceId == null) {
    // Override default window.close.
    window.close = function () {
      ipcRendererInternal.send('ELECTRON_BROWSER_WINDOW_CLOSE');
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['close'], window.close);
  }

  if (!usesNativeWindowOpen) {
    // TODO(MarshallOfSound): Make compatible with ctx isolation without hole-punch
    // Make the browser window or guest view emit "new-window" event.
    (window as any).open = function (url?: string, frameName?: string, features?: string) {
      if (url != null && url !== '') {
        url = resolveURL(url, location.href);
      }
      const guestId = ipcRendererInternal.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, toString(frameName), toString(features));
      if (guestId != null) {
        return getOrCreateProxy(guestId);
      } else {
        return null;
      }
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueWithDynamicPropsFromIsolatedWorld(['open'], window.open);
  }

  if (openerId != null) {
    window.opener = getOrCreateProxy(openerId);
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueWithDynamicPropsFromIsolatedWorld(['opener'], window.opener);
  }

  // But we do not support prompt().
  window.prompt = function () {
    throw new Error('prompt() is and will not be supported.');
  };
  if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['prompt'], window.prompt);

  if (!usesNativeWindowOpen || openerId != null) {
    ipcRendererInternal.on('ELECTRON_GUEST_WINDOW_POSTMESSAGE', function (
      _event, sourceId: number, message: any, sourceOrigin: string
    ) {
      // Manually dispatch event instead of using postMessage because we also need to
      // set event.source.
      //
      // Why any? We can't construct a MessageEvent and we can't
      // use `as MessageEvent` because you're not supposed to override
      // data, origin, and source
      const event: any = document.createEvent('Event');
      event.initEvent('message', false, false);

      event.data = message;
      event.origin = sourceOrigin;
      event.source = getOrCreateProxy(sourceId);

      window.dispatchEvent(event as MessageEvent);
    });
  }

  if (!process.sandboxed && !rendererProcessReuseEnabled) {
    window.history.back = function () {
      ipcRendererInternal.send('ELECTRON_NAVIGATION_CONTROLLER_GO_BACK');
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['history', 'back'], window.history.back);

    window.history.forward = function () {
      ipcRendererInternal.send('ELECTRON_NAVIGATION_CONTROLLER_GO_FORWARD');
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['history', 'forward'], window.history.forward);

    window.history.go = function (offset: number) {
      ipcRendererInternal.send('ELECTRON_NAVIGATION_CONTROLLER_GO_TO_OFFSET', +offset);
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['history', 'go'], window.history.go);

    const getHistoryLength = () => ipcRendererInternal.sendSync('ELECTRON_NAVIGATION_CONTROLLER_LENGTH');
    Object.defineProperty(window.history, 'length', {
      get: getHistoryLength,
      set () {}
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['history', 'length'], getHistoryLength);
  }

  if (guestInstanceId != null) {
    // Webview `document.visibilityState` tracks window visibility (and ignores
    // the actual <webview> element visibility) for backwards compatibility.
    // See discussion in #9178.
    //
    // Note that this results in duplicate visibilitychange events (since
    // Chromium also fires them) and potentially incorrect visibility change.
    // We should reconsider this decision for Electron 2.0.
    let cachedVisibilityState = isHiddenPage ? 'hidden' : 'visible';

    // Subscribe to visibilityState changes.
    ipcRendererInternal.on('ELECTRON_GUEST_INSTANCE_VISIBILITY_CHANGE', function (_event, visibilityState: VisibilityState) {
      if (cachedVisibilityState !== visibilityState) {
        cachedVisibilityState = visibilityState;
        document.dispatchEvent(new Event('visibilitychange'));
      }
    });

    // Make document.hidden and document.visibilityState return the correct value.
    const getDocumentHidden = () => cachedVisibilityState !== 'visible';
    Object.defineProperty(document, 'hidden', {
      get: getDocumentHidden
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['document', 'hidden'], getDocumentHidden);

    const getDocumentVisibilityState = () => cachedVisibilityState;
    Object.defineProperty(document, 'visibilityState', {
      get: getDocumentVisibilityState
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['document', 'visibilityState'], getDocumentVisibilityState);
  }
};
