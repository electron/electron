'use strict'

const EventEmitter = require('events').EventEmitter

const webFrame = process.atomBinding('web_frame').webFrame

// webFrame is an EventEmitter.
Object.setPrototypeOf(webFrame.__proto__, EventEmitter.prototype)

// Lots of webview would subscribe to webFrame's events.
webFrame.setMaxListeners(0)

module.exports = webFrame
