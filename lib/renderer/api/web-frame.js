'use strict'

const { EventEmitter } = require('events')
const { deprecate } = require('electron')
const { webFrame, WebFrame } = process.atomBinding('web_frame')

// WebFrame is an EventEmitter.
Object.setPrototypeOf(WebFrame.prototype, EventEmitter.prototype)
EventEmitter.call(webFrame)

// Lots of webview would subscribe to webFrame's events.
webFrame.setMaxListeners(0)

WebFrame.prototype.registerURLSchemeAsBypassingCSP = function (scheme) {
  // TODO (nitsakh): Remove in 5.0
  deprecate.log('webFrame.registerURLSchemeAsBypassingCSP is deprecated. Use app api starting in 5.0')

  return this._registerURLSchemeAsBypassingCSP(scheme)
}

WebFrame.prototype.registerURLSchemeAsPrivileged = function (scheme, options) {
  // TODO (nitsakh): Remove in 5.0
  deprecate.log('webFrame.registerURLSchemeAsPrivileged is deprecated. Use app api starting in 5.0')

  return this._registerURLSchemeAsPrivileged(scheme, options)
}

module.exports = webFrame
