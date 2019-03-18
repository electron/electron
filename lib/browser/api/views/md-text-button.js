'use strict'

const electron = require('electron')

const { LabelButton } = electron
const { MdTextButton } = process.electronBinding('md_text_button')

Object.setPrototypeOf(MdTextButton.prototype, LabelButton.prototype)

MdTextButton.prototype._init = function () {
  // Call parent class's _init.
  LabelButton.prototype._init.call(this)
}

module.exports = MdTextButton
