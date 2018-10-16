'use strict'

const { EventEmitter } = require('events')
const { BrowserView } = process.atomBinding('browser_view')

Object.setPrototypeOf(BrowserView.prototype, EventEmitter.prototype)

BrowserView.fromWebContents = (webContents) => {
  for (const view of BrowserView.getAllViews()) {
    if (view.webContents.equal(webContents)) return view
  }

  return null
}

module.exports = BrowserView
