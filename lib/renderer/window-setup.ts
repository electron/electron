// This file should have no requires since it is used by the isolated context
// preload bundle. Instead arguments should be passed in for everything it
// needs.

// This file implements the following APIs:
// - window.history.back()
// - window.history.forward()
// - window.history.go()
// - window.history.length
// - window.open()
// - window.opener.blur()
// - window.opener.close()
// - window.opener.eval()
// - window.opener.focus()
// - window.opener.location
// - window.opener.print()
// - window.opener.postMessage()
// - window.prompt()
// - document.hidden
// - document.visibilityState

const { defineProperty, defineProperties } = Object

// Helper function to resolve relative url.
const a = window.document.createElement('a')
const resolveURL = function (url: string) {
  a.href = url
  return a.href
}

// Use this method to ensure values expected as strings in the main process
// are convertible to strings in the renderer process. This ensures exceptions
// converting values to strings are thrown in this process.
const toString = (value: any) => {
  return value != null ? `${value}` : value
}

const windowProxies: Record<number, BrowserWindowProxy> = {}

const getOrCreateProxy = (ipcRenderer: Electron.IpcRenderer, guestId: number) => {
  let proxy = windowProxies[guestId]
  if (proxy == null) {
    proxy = new BrowserWindowProxy(ipcRenderer, guestId)
    windowProxies[guestId] = proxy
  }
  return proxy
}

const removeProxy = (guestId: number) => {
  delete windowProxies[guestId]
}

enum LocationProperties {
  hash = 'hash',
  href = 'href',
  host = 'host',
  hostname = 'hostname',
  origin = 'origin',
  pathname = 'pathname',
  port = 'port',
  protocol = 'protocol',
  search = 'search'
}

class LocationProxy {
  // Defined via defineProperties(), which is hard for TS to understand.
  // We therefore help it with the "!" expression.
  public hash!: string;
  public href!: string;
  public host!: string;
  public hostname!: string;
  public origin!: string;
  public pathname!: string;
  public port!: string;
  public protocol!: string;
  public search!: URLSearchParams;

  constructor(ipcRenderer: Electron.IpcRenderer, guestId: number) {
    function getGuestURL(): URL | null {
      const urlString = ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', guestId, 'getURL')
      try {
        return new URL(urlString)
      } catch (e) {
        console.error('LocationProxy: failed to parse string', urlString, e)
      }
  
      return null
    }
  
    function propertyProxyFor<T>(property: LocationProperties) {
      return {
        get: function (): T | string {
          const guestURL = getGuestURL()
          const value = guestURL ? guestURL[property] : ''
          return value === undefined ? '' : value
        },
        set: function (newVal: T) {
          const guestURL = getGuestURL()
          if (guestURL) {
            // TypeScript doesn't want us to assign to read-only variables.
            // It's right, that's bad, but we're doing it anway.
            (guestURL as any)[property] = newVal
  
            return ipcRenderer.sendSync(
              'ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC',
              guestId, 'loadURL', guestURL.toString())
          }
        }
      }
    }
  
    defineProperties(this, {
      hash: propertyProxyFor<string>(LocationProperties.hash),
      href: propertyProxyFor<string>(LocationProperties.href),
      host: propertyProxyFor<string>(LocationProperties.host),
      hostname: propertyProxyFor<string>(LocationProperties.hostname),
      origin: propertyProxyFor<string>(LocationProperties.origin),
      pathname: propertyProxyFor<string>(LocationProperties.pathname),
      port: propertyProxyFor<string>(LocationProperties.port),
      protocol: propertyProxyFor<string>(LocationProperties.protocol),
      search: propertyProxyFor<URL>(LocationProperties.search)
    })
  }

  public toString(): string {
    return this.href
  }
}

class BrowserWindowProxy {
  public closed: boolean = false

  private _location: LocationProxy

  // TypeScript doesn't allow getters/accessors with different types,
  // so for now, we'll have to make do with an "any" in the mix.
  // https://github.com/Microsoft/TypeScript/issues/2521
  public get location(): LocationProxy | any {
    return this._location
  }
  public set location(url: string | any) {
    url = resolveURL(url)
    this.ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD_SYNC', this.guestId, 'loadURL', url)
  }

  constructor (private ipcRenderer: Electron.IpcRenderer, private guestId: any) {
    this._location = new LocationProxy(ipcRenderer, guestId)

    ipcRenderer.once(`ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_${guestId}`, () => {
      removeProxy(guestId)
      this.closed = true
    })
  }
  
  public close() {
    this.ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', this.guestId)
  }

  public focus() {
    this.ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'focus')
  }

  public blur() {
    this.ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD', this.guestId, 'blur')
  }

  public print() {
    this.ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, 'print')
  }

  public postMessage(message: any, targetOrigin: any) {
    this.ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', this.guestId, message, toString(targetOrigin), window.location.origin)
  }

  public eval(...args: any[]) {
    this.ipcRenderer.send('ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD', this.guestId, 'executeJavaScript', ...args)
  }
}

const windowSetup = (
  ipcRenderer: Electron.IpcRenderer, guestInstanceId: number, openerId: number, isHiddenPage: boolean, usesNativeWindowOpen: boolean
) => {
  if (guestInstanceId == null) {
    // Override default window.close.
    window.close = function () {
      ipcRenderer.sendSync('ELECTRON_BROWSER_WINDOW_CLOSE')
    }
  }

  if (!usesNativeWindowOpen) {
    // Make the browser window or guest view emit "new-window" event.
    (window as any).open = function (url?: string, frameName?: string, features?: string) {
      if (url != null && url !== '') {
        url = resolveURL(url)
      }
      const guestId = ipcRenderer.sendSync('ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN', url, toString(frameName), toString(features))
      if (guestId != null) {
        return getOrCreateProxy(ipcRenderer, guestId)
      } else {
        return null
      }
    }

    if (openerId != null) {
      window.opener = getOrCreateProxy(ipcRenderer, openerId)
    }
  }

  // But we do not support prompt().
  window.prompt = function () {
    throw new Error('prompt() is and will not be supported.')
  }

  ipcRenderer.on('ELECTRON_GUEST_WINDOW_POSTMESSAGE', function (
    _event: Event, sourceId: number, message: any, sourceOrigin: string
  ) {
    // Manually dispatch event instead of using postMessage because we also need to
    // set event.source.
    const messageEvent = new MessageEvent('message', {
      data: message,
      origin: sourceOrigin,
      source: getOrCreateProxy(ipcRenderer, sourceId) as any as Window,
      bubbles: false,
      cancelable: false
    })

    window.dispatchEvent(messageEvent as MessageEvent)
  })

  window.history.back = function () {
    ipcRenderer.send('ELECTRON_NAVIGATION_CONTROLLER_GO_BACK')
  }

  window.history.forward = function () {
    ipcRenderer.send('ELECTRON_NAVIGATION_CONTROLLER_GO_FORWARD')
  }

  window.history.go = function (offset: number) {
    ipcRenderer.send('ELECTRON_NAVIGATION_CONTROLLER_GO_TO_OFFSET', +offset)
  }

  defineProperty(window.history, 'length', {
    get: function () {
      return ipcRenderer.sendSync('ELECTRON_NAVIGATION_CONTROLLER_LENGTH')
    }
  })

  if (guestInstanceId != null) {
    // Webview `document.visibilityState` tracks window visibility (and ignores
    // the actual <webview> element visibility) for backwards compatibility.
    // See discussion in #9178.
    //
    // Note that this results in duplicate visibilitychange events (since
    // Chromium also fires them) and potentially incorrect visibility change.
    // We should reconsider this decision for Electron 2.0.
    let cachedVisibilityState = isHiddenPage ? 'hidden' : 'visible'

    // Subscribe to visibilityState changes.
    ipcRenderer.on('ELECTRON_GUEST_INSTANCE_VISIBILITY_CHANGE', function (_event: Event, visibilityState: VisibilityState) {
      if (cachedVisibilityState !== visibilityState) {
        cachedVisibilityState = visibilityState
        document.dispatchEvent(new Event('visibilitychange'))
      }
    })

    // Make document.hidden and document.visibilityState return the correct value.
    defineProperty(document, 'hidden', {
      get: function () {
        return cachedVisibilityState !== 'visible'
      }
    })

    defineProperty(document, 'visibilityState', {
      get: function () {
        return cachedVisibilityState
      }
    })
  }
}

export default windowSetup
