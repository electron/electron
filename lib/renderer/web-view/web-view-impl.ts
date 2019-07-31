import { remote, webFrame } from 'electron'

import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils'
import * as guestViewInternal from '@electron/internal/renderer/web-view/guest-view-internal'
import { WEB_VIEW_CONSTANTS } from '@electron/internal/renderer/web-view/web-view-constants'
import { syncMethods, asyncMethods } from '@electron/internal/common/web-view-methods'

const v8Util = process.electronBinding('v8_util')

// ID generator.
let nextId = 0

const getNextId = function () {
  return ++nextId
}

// Represents the internal state of the WebView node.
export class WebViewImpl {
  public beforeFirstNavigation = true
  public elementAttached = false
  public guestInstanceId?: number
  public hasFocus = false
  public internalInstanceId?: number;
  public resizeObserver?: ResizeObserver;
  public userAgentOverride?: string;
  public viewInstanceId: number

  // on* Event handlers.
  public on: Record<string, any> = {}
  public internalElement: HTMLIFrameElement

  // Replaced in web-view-attributes
  public attributes: Record<string, any> = {}
  public setupWebViewAttributes (): void {}

  constructor (public webviewNode: HTMLElement) {
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
      this.guestInstanceId = void 0
    }

    this.beforeFirstNavigation = true
    this.attributes[WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION].validPartitionId = true

    // Since attachment swaps a local frame for a remote frame, we need our
    // internal iframe element to be local again before we can reattach.
    const newFrame = this.createInternalElement()
    const oldFrame = this.internalElement
    this.internalElement = newFrame

    if (oldFrame && oldFrame.parentNode) {
      oldFrame.parentNode.replaceChild(newFrame, oldFrame)
    }
  }

  // This observer monitors mutations to attributes of the <webview> and
  // updates the BrowserPlugin properties accordingly. In turn, updating
  // a BrowserPlugin property will update the corresponding BrowserPlugin
  // attribute, if necessary. See BrowserPlugin::UpdateDOMAttribute for more
  // details.
  handleWebviewAttributeMutation (attributeName: string, oldValue: any, newValue: any) {
    if (!this.attributes[attributeName] || this.attributes[attributeName].ignoreMutation) {
      return
    }

    // Let the changed attribute handle its own mutation
    this.attributes[attributeName].handleMutation(oldValue, newValue)
  }

  onElementResize () {
    const resizeEvent = new Event('resize') as ElectronInternal.WebFrameResizeEvent
    resizeEvent.newWidth = this.webviewNode.clientWidth
    resizeEvent.newHeight = this.webviewNode.clientHeight
    this.dispatchEvent(resizeEvent)
  }

  createGuest () {
    guestViewInternal.createGuest(this.buildParams()).then(guestInstanceId => {
      this.attachGuestInstance(guestInstanceId)
    })
  }

  createGuestSync () {
    this.beforeFirstNavigation = false
    this.attachGuestInstance(guestViewInternal.createGuestSync(this.buildParams()))
  }

  dispatchEvent (webViewEvent: Electron.Event) {
    this.webviewNode.dispatchEvent(webViewEvent)
  }

  // Adds an 'on<event>' property on the webview, which can be used to set/unset
  // an event handler.
  setupEventProperty (eventName: string) {
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
  onLoadCommit (webViewEvent: ElectronInternal.WebViewEvent) {
    const oldValue = this.webviewNode.getAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC)
    const newValue = webViewEvent.url
    if (webViewEvent.isMainFrame && (oldValue !== newValue)) {
      // Touching the src attribute triggers a navigation. To avoid
      // triggering a page reload on every guest-initiated navigation,
      // we do not handle this mutation.
      this.attributes[WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC].setValueIgnoreMutation(newValue)
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

  onAttach (storagePartitionId: number) {
    return this.attributes[WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION].setValue(storagePartitionId)
  }

  buildParams () {
    const params: Record<string, any> = {
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

  attachGuestInstance (guestInstanceId: number) {
    if (!this.elementAttached) {
      // The element could be detached before we got response from browser.
      return
    }
    this.internalInstanceId = getNextId()
    this.guestInstanceId = guestInstanceId

    guestViewInternal.attachGuest(
      this.internalInstanceId,
      this.guestInstanceId,
      this.buildParams(),
      this.internalElement.contentWindow!
    )

    // ResizeObserver is a browser global not recognized by "standard".
    /* globals ResizeObserver */
    // TODO(zcbenz): Should we deprecate the "resize" event? Wait, it is not
    // even documented.
    this.resizeObserver = new ResizeObserver(this.onElementResize.bind(this))
    this.resizeObserver.observe(this.internalElement)
  }
}

export const setupAttributes = () => {
  require('@electron/internal/renderer/web-view/web-view-attributes')
}

// I wish eslint wasn't so stupid, but it is
// eslint-disable-next-line
export const setupMethods = (WebViewElement: typeof ElectronInternal.WebViewElement) => {
  WebViewElement.prototype.getWebContentsId = function () {
    const internal = v8Util.getHiddenValue<WebViewImpl>(this, 'internal')
    if (!internal.guestInstanceId) {
      throw new Error('The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.')
    }
    return internal.guestInstanceId
  }

  // WebContents associated with this webview.
  WebViewElement.prototype.getWebContents = function () {
    if (!remote) {
      throw new Error('getGuestWebContents requires remote, which is not enabled')
    }
    const internal = v8Util.getHiddenValue<WebViewImpl>(this, 'internal')
    if (!internal.guestInstanceId) {
      internal.createGuestSync()
    }

    return (remote as Electron.RemoteInternal).getGuestWebContents(internal.guestInstanceId!)
  }

  // Focusing the webview should move page focus to the underlying iframe.
  WebViewElement.prototype.focus = function () {
    this.contentWindow.focus()
  }

  // Forward proto.foo* method calls to WebViewImpl.foo*.
  const createBlockHandler = function (method: string) {
    return function (this: ElectronInternal.WebViewElement, ...args: Array<any>) {
      return ipcRendererUtils.invokeSync('ELECTRON_GUEST_VIEW_MANAGER_CALL', this.getWebContentsId(), method, args)
    }
  }

  for (const method of syncMethods) {
    (WebViewElement.prototype as Record<string, any>)[method] = createBlockHandler(method)
  }

  const createNonBlockHandler = function (method: string) {
    return function (this: ElectronInternal.WebViewElement, ...args: Array<any>) {
      return ipcRendererInternal.invoke('ELECTRON_GUEST_VIEW_MANAGER_CALL', this.getWebContentsId(), method, args)
    }
  }

  for (const method of asyncMethods) {
    (WebViewElement.prototype as Record<string, any>)[method] = createNonBlockHandler(method)
  }
}

export const webViewImplModule = {
  setupAttributes,
  setupMethods,
  guestViewInternal,
  webFrame,
  WebViewImpl
}
