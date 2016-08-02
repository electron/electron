'use strict'

const {EventEmitter} = require('events')
const {webFrame, WebFrame} = process.atomBinding('web_frame')

// WebFrame is an EventEmitter.
Object.setPrototypeOf(WebFrame.prototype, EventEmitter.prototype)

// Lots of webview would subscribe to webFrame's events.
webFrame.setMaxListeners(0)

module.exports = webFrame
