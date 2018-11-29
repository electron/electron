'use strict'

const electron = require('electron')

const { View } = electron
const { ResizeArea } = process.atomBinding('resize_area')

Object.setPrototypeOf(ResizeArea.prototype, View.prototype)

ResizeArea.prototype._init = function () {
  // Call parent class's _init.
  View.prototype._init.call(this)
}

module.exports = ResizeArea
