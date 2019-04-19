'use strict'

const { EventEmitter } = require('events')
const binding = process.atomBinding('web_frame')
const { deprecate } = require('electron')

class WebFrame extends EventEmitter {
  constructor (context) {
    super()

    this.context = context
    // Lots of webview would subscribe to webFrame's events.
    this.setMaxListeners(0)
  }

  findFrameByRoutingId (...args) {
    return getWebFrame(binding._findFrameByRoutingId(this.context, ...args))
  }

  getFrameForSelector (...args) {
    return getWebFrame(binding._getFrameForSelector(this.context, ...args))
  }

  findFrameByName (...args) {
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
  setIsolatedWorldSecurityOrigin (worldId, securityOrigin) {
    deprecate.warn('webFrame.setIsolatedWorldSecurityOrigin', 'webFrame.setIsolatedWorldInfo')
    binding.setIsolatedWorldInfo(this.context, worldId, { securityOrigin })
  }

  setIsolatedWorldContentSecurityPolicy (worldId, csp) {
    deprecate.warn('webFrame.setIsolatedWorldContentSecurityPolicy', 'webFrame.setIsolatedWorldInfo')
    binding.setIsolatedWorldInfo(this.context, worldId, {
      securityOrigin: window.location.origin,
      csp
    })
  }

  setIsolatedWorldHumanReadableName (worldId, name) {
    deprecate.warn('webFrame.setIsolatedWorldHumanReadableName', 'webFrame.setIsolatedWorldInfo')
    binding.setIsolatedWorldInfo(this.context, worldId, { name })
  }
}

// Populate the methods.
for (const name in binding) {
  if (!name.startsWith('_')) { // some methods are manully populated above
    WebFrame.prototype[name] = function (...args) {
      return binding[name](this.context, ...args)
    }
  }
}

// Helper to return WebFrame or null depending on context.
// TODO(zcbenz): Consider returning same WebFrame for the same frame.
function getWebFrame (context) {
  return context ? new WebFrame(context) : null
}

module.exports = new WebFrame(window)
