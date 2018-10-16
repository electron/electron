'use strict'

const electron = require('electron')

const { View } = electron
const { WebContentsView } = process.atomBinding('web_contents_view')

Object.setPrototypeOf(WebContentsView.prototype, View.prototype)

WebContentsView.prototype._init = function () {
  // Call parent class's _init.
  View.prototype._init.call(this)
}

module.exports = WebContentsView
