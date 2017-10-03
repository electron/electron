'use strict'

const {EventEmitter} = require('events')
const {BrowserView} = process.atomBinding('browser_view')

Object.setPrototypeOf(BrowserView.prototype, EventEmitter.prototype)

module.exports = BrowserView
