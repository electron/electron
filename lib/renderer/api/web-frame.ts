import { EventEmitter } from 'events'
import { deprecate } from 'electron'

const binding = process.atomBinding('web_frame')

class WebFrame extends EventEmitter {
  constructor (public context: Window) {
    super()

    // Lots of webview would subscribe to webFrame's events.
    this.setMaxListeners(0)
  }

  findFrameByRoutingId (...args: Array<any>) {
    return getWebFrame(binding._findFrameByRoutingId(this.context, ...args))
  }

  getFrameForSelector (...args: Array<any>) {
    return getWebFrame(binding._getFrameForSelector(this.context, ...args))
  }

  findFrameByName (...args: Array<any>) {
    return getWebFrame(binding._findFrameByName(this.context, ...args))
  }

  get opener () {
    return getWebFrame(binding._getOpener(this.context))
  }

  get parent () {
    return getWebFrame(binding._getParent(this.context))
  }

  get top () {
    return getWebFrame(binding._getTop(this.context))
  }

  get firstChild () {
    return getWebFrame(binding._getFirstChild(this.context))
  }

  get nextSibling () {
    return getWebFrame(binding._getNextSibling(this.context))
  }

  get routingId () {
    return binding._getRoutingId(this.context)
  }

  // Deprecations
  // TODO(nitsakh): Remove in 6.0
  setIsolatedWorldSecurityOrigin (worldId: number, securityOrigin: string) {
    deprecate.warn('webFrame.setIsolatedWorldSecurityOrigin', 'webFrame.setIsolatedWorldInfo')
    binding.setIsolatedWorldInfo(this.context, worldId, { securityOrigin })
  }

  setIsolatedWorldContentSecurityPolicy (worldId: number, csp: string) {
    deprecate.warn('webFrame.setIsolatedWorldContentSecurityPolicy', 'webFrame.setIsolatedWorldInfo')
    binding.setIsolatedWorldInfo(this.context, worldId, {
      securityOrigin: window.location.origin,
      csp
    })
  }

  setIsolatedWorldHumanReadableName (worldId: number, name: string) {
    deprecate.warn('webFrame.setIsolatedWorldHumanReadableName', 'webFrame.setIsolatedWorldInfo')
    binding.setIsolatedWorldInfo(this.context, worldId, { name })
  }
}

// Populate the methods.
for (const name in binding) {
  if (!name.startsWith('_')) { // some methods are manually populated above
    // TODO(felixrieseberg): Once we can type web_frame natives, we could
    // use a neat `keyof` here
    (WebFrame as any).prototype[name] = function (...args: Array<any>) {
      return binding[name](this.context, ...args)
    }
  }
}

// Helper to return WebFrame or null depending on context.
// TODO(zcbenz): Consider returning same WebFrame for the same frame.
function getWebFrame (context: Window) {
  return context ? new WebFrame(context) : null
}

const promisifiedMethods = new Set<string>([
  'executeJavaScript',
  'executeJavaScriptInIsolatedWorld'
])

for (const method of promisifiedMethods) {
  (WebFrame as any).prototype[method] = deprecate.promisify((WebFrame as any).prototype[method])
}

const _webFrame = new WebFrame(window)

export default _webFrame
