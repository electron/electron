'use strict'

const electron = require('electron')

const { View } = electron
const { TextField } = process.atomBinding('text_field')

Object.setPrototypeOf(TextField.prototype, View.prototype)

TextField.prototype._init = function () {
  // Call parent class's _init.
  View.prototype._init.call(this)
}

module.exports = TextField
