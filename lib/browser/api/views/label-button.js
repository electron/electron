'use strict'

const electron = require('electron')

const { Button } = electron
const { LabelButton } = process.atomBinding('label_button')

Object.setPrototypeOf(LabelButton.prototype, Button.prototype)

LabelButton.prototype._init = function () {
  // Call parent class's _init.
  Button.prototype._init.call(this)
}

module.exports = LabelButton
