'use strict'

const electron = require('electron')

const {View} = electron
const {Button} = process.atomBinding('button')

Object.setPrototypeOf(Button.prototype, View.prototype)

Button.prototype._init = function () {
  // Call parent class's _init.
  View.prototype._init.call(this)
}

module.exports = Button
