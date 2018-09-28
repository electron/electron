'use strict'

const electron = require('electron')

const { LayoutManager } = electron
const { BoxLayout } = process.atomBinding('box_layout')

Object.setPrototypeOf(BoxLayout.prototype, LayoutManager.prototype)

BoxLayout.prototype._init = function () {
  // Call parent class's _init.
  LayoutManager.prototype._init.call(this)
}

module.exports = BoxLayout
