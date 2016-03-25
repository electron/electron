'use strict'

const deprecate = require('electron').deprecate
const EventEmitter = require('events').EventEmitter

const webFrame = process.atomBinding('web_frame').webFrame

// webFrame is an EventEmitter.
webFrame.__proto__ = EventEmitter.prototype

// Lots of webview would subscribe to webFrame's events.
webFrame.setMaxListeners(0)

// Deprecated.
deprecate.rename(webFrame, 'registerUrlSchemeAsSecure', 'registerURLSchemeAsSecure')
deprecate.rename(webFrame, 'registerUrlSchemeAsBypassingCSP', 'registerURLSchemeAsBypassingCSP')
deprecate.rename(webFrame, 'registerUrlSchemeAsPrivileged', 'registerURLSchemeAsPrivileged')

module.exports = webFrame
